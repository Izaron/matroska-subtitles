#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

/* EBML header ids */
#define EBML_EBML_HEADER 0x1A45DFA3
#define EBML_EBML_VERSION 0x4286
#define EBML_EBML_READ_VERSION 0x42F7
#define EBML_EBML_MAX_ID_LENGTH 0x42F2
#define EBML_EBML_MAX_SIZE_LENGTH 0x42F3
#define EBML_EBML_DOC_TYPE 0x4282
#define EBML_EBML_DOC_TYPE_VERSION 0x4287
#define EBML_EBML_DOC_TYPE_READ_VERSION 0x4285

/* Segment ids */
#define EBML_SEGMENT_HEADER 0x18538067
#define EBML_SEGMENT_SEEK_HEAD 0x114D9B74
#define EBML_SEGMENT_INFO 0x1549A966
#define EBML_SEGMENT_CLUSTER 0x1F43B675
#define EBML_SEGMENT_TRACKS 0x1654AE6B
#define EBML_SEGMENT_CUES 0x1C53BB6B
#define EBML_SEGMENT_ATTACHMENTS 0x1941A469
#define EBML_SEGMENT_CHAPTERS 0x1043A770
#define EBML_SEGMENT_TAGS 0x1254C367

/* Segment info ids */
#define EBML_SEGMENT_INFO_SEGMENT_UID 0x73A4
#define EBML_SEGMENT_INFO_SEGMENT_FILENAME 0x7384
#define EBML_SEGMENT_INFO_PREV_UID 0x3CB923
#define EBML_SEGMENT_INFO_PREV_FILENAME 0x3C83AB
#define EBML_SEGMENT_INFO_NEXT_UID 0x3EB923
#define EBML_SEGMENT_INFO_NEXT_FILENAME 0x3E83BB
#define EBML_SEGMENT_INFO_SEGMENT_FAMILY 0x4444
#define EBML_SEGMENT_INFO_CHAPTER_TRANSLATE 0x6924
#define EBML_SEGMENT_INFO_TIMECODE_SCALE 0x2AD7B1
#define EBML_SEGMENT_INFO_DURATION 0x4489
#define EBML_SEGMENT_INFO_DATE_UTC 0x4461
#define EBML_SEGMENT_INFO_TITLE 0x7BA9
#define EBML_SEGMENT_MUXING_APP 0x4D80
#define EBML_SEGMENT_WRITING_APP 0x5741

/* Segment tracks ids */
#define EBML_SEGMENT_TRACK_ENTRY 0xAE
#define EBML_SEGMENT_TRACK_TRACK_NUMBER 0xD7
#define EBML_SEGMENT_TRACK_TRACK_UID 0x73C5
#define EBML_SEGMENT_TRACK_TRACK_TYPE 0x83
#define EBML_SEGMENT_TRACK_FLAG_ENABLED 0xB9
#define EBML_SEGMENT_TRACK_FLAG_DEFAULT 0x88
#define EBML_SEGMENT_TRACK_FLAG_FORCED 0x55AA
#define EBML_SEGMENT_TRACK_FLAG_LACING 0x9C
#define EBML_SEGMENT_TRACK_MIN_CACHE 0x6DE7
#define EBML_SEGMENT_TRACK_MAX_CACHE 0x6DF8
#define EBML_SEGMENT_TRACK_DEFAULT_DURATION 0x23E383
#define EBML_SEGMENT_TRACK_DEFAULT_DECODED_FIELD_DURATION 0x234E7A
#define EBML_SEGMENT_TRACK_MAX_BLOCK_ADDITION_ID 0x55EE
#define EBML_SEGMENT_TRACK_NAME 0x536E
#define EBML_SEGMENT_TRACK_LANGUAGE 0x22B59C
#define EBML_SEGMENT_TRACK_CODEC_ID 0x86
#define EBML_SEGMENT_TRACK_CODEC_PRIVATE 0x63A2
#define EBML_SEGMENT_TRACK_CODEC_NAME 0x258688
#define EBML_SEGMENT_TRACK_CODEC_ATTACHMENT_LINK 0x7446
#define EBML_SEGMENT_TRACK_CODEC_DECODE_ALL 0xAA
#define EBML_SEGMENT_TRACK_TRACK_OVERLAY 0x6FAB
#define EBML_SEGMENT_TRACK_CODEC_DELAY 0x56AA
#define EBML_SEGMENT_TRACK_SEEK_PRE_ROLL 0x56BB
#define EBML_SEGMENT_TRACK_TRACK_TRANSLATE 0x6624
#define EBML_SEGMENT_TRACK_VIDEO 0xE0
#define EBML_SEGMENT_TRACK_AUDIO 0xE1
#define EBML_SEGMENT_TRACK_TRACK_OPERATION 0xE2
#define EBML_SEGMENT_TRACK_CONTENT_ENCODINGS 0x6D80
/* DEFENCE FROM THE FOOL - there are can deprecated ids in using ... */
#define EBML_SEGMENT_TRACK_TRACK_TIMECODE_SCALE 0x23314F
#define EBML_SEGMENT_TRACK_TRACK_OFFSET 0x537F

/* Misc ids */
#define EBML_VOID 0xEC
#define EBML_CRC32 0xBF

/* Other defines */
#define EBML_MAX_ID_LENGTH 4

/* Messages */
#define EBML_INFO "Matroska parser info: "
#define EBML_WARNING "Matroska parser warning: "
#define EBML_ERROR "Matroska parser error: "

/* Boilerplate code */
#define EBML_SWITCH_BREAK(a,b) (a)=0;(b)=0;break


void skip_bytes(FILE* file, long n) {
    fseek(file, n, SEEK_CUR);
}

void set_bytes(FILE* file, long n) {
    fseek(file, n, SEEK_SET);
}

long get_current_byte(FILE* file) {
    return ftell(file);
}

unsigned char* read_bytes(FILE* file, size_t n) {
    unsigned char* buffer = malloc(sizeof(unsigned char) * n);
    fread(buffer, 1, n, file);
    return buffer;
}

unsigned char read_byte(FILE* file) {
    unsigned char ch;
    fread(&ch, 1, 1, file);
    return ch;
}

unsigned long read_vint_length(FILE* file) {
    unsigned char ch = read_byte(file);
    int cnt = 1;
    for (int i = 7; i >= 0; i--) {
        if ((ch & (1 << i)) != 0) {
            ch ^= (1 << i);
            break;
        } else
            cnt++;
    }
    unsigned long ret = ch;
    for (int i = 1; i < cnt; i++) {
        ret <<= 8;
        ret += read_byte(file);
    }
    return ret;
}

unsigned char* read_vint_block(FILE* file) {
    unsigned long len = read_vint_length(file);
    return read_bytes(file, len);
}

unsigned long read_vint_block_int(FILE* file) {
    unsigned long len = read_vint_length(file);
    unsigned char* s = read_bytes(file, len);

    unsigned long res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 8;
        res += s[i];
    }

    return res;
}

unsigned char* read_vint_block_string(FILE* file) {
    return read_vint_block(file);
}

void read_vint_block_skip(FILE* file) {
    long len = read_vint_length(file);
    skip_bytes(file, len);
}

void parse_ebml(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* EBML ids */
            case EBML_EBML_VERSION:
                read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_EBML_READ_VERSION:
                read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_EBML_MAX_ID_LENGTH:
                read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_EBML_MAX_SIZE_LENGTH:
                read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_EBML_DOC_TYPE:
                printf("Document type: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_EBML_DOC_TYPE_VERSION:
                read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_EBML_DOC_TYPE_READ_VERSION:
                read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case EBML_VOID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_CRC32:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == EBML_MAX_ID_LENGTH) {
                    printf (EBML_ERROR "Unknown ID 0x%x, skipping EBML block\n", code);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment_info(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment info ids */
            case EBML_SEGMENT_INFO_SEGMENT_UID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_SEGMENT_FILENAME:
                printf("Filename: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_PREV_UID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_PREV_FILENAME:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_NEXT_UID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_NEXT_FILENAME:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_SEGMENT_FAMILY:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_CHAPTER_TRANSLATE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_TIMECODE_SCALE:
                printf("Timecode scale: %ld\n", read_vint_block_int(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_DURATION:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_DATE_UTC:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO_TITLE:
                printf("Title: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_MUXING_APP:
                printf("Muxing app: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_WRITING_APP:
                printf("Writing app: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case EBML_VOID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_CRC32:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == EBML_MAX_ID_LENGTH) {
                    printf (EBML_ERROR "Unknown ID 0x%x, skipping segment info block\n", code);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* get_track_entry_type_description(long type) {
    switch (type) {
        case 1:
            return "video";
        case 2:
            return "audio";
        case 3:
            return "complex";
        case 0x10:
            return "logo";
        case 0x11:
            return "subtitle";
        case 0x12:
            return "buttons";
        case 0x20:
            return "control";
        default:
            return NULL;
    }
}

void parse_segment_track_entry(FILE* file) {
    printf("\n==== Track entry ====\n");

    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Track entry ids*/
            case EBML_SEGMENT_TRACK_TRACK_NUMBER:
                printf("Number: %ld\n", read_vint_block_int(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_UID:
                printf("UID: %lu\n", read_vint_block_int(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_TYPE:
                printf("Type: %s\n", get_track_entry_type_description(read_vint_block_int(file)));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_FLAG_ENABLED:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_FLAG_DEFAULT:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_FLAG_FORCED:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_FLAG_LACING:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_MIN_CACHE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_MAX_CACHE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_DEFAULT_DURATION:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_DEFAULT_DECODED_FIELD_DURATION:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_MAX_BLOCK_ADDITION_ID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_NAME:
                printf("Name: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_LANGUAGE:
                printf("Language: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CODEC_ID:
                printf("Codec ID: %s\n", read_vint_block_string(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CODEC_PRIVATE:
                // WARNING - this string can contain headers for some formats of subtitles!
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CODEC_NAME:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CODEC_ATTACHMENT_LINK:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CODEC_DECODE_ALL:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_OVERLAY:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CODEC_DELAY:
                printf("Codec Delay: %ld\n", read_vint_block_int(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_SEEK_PRE_ROLL:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_TRANSLATE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_VIDEO:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_AUDIO:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_OPERATION:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_CONTENT_ENCODINGS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);

            /* Deprecated IDs */
            case EBML_SEGMENT_TRACK_TRACK_TIMECODE_SCALE:
                printf(EBML_WARNING "deprecated ID 0x%x on position %ld\n", code, get_current_byte(file));
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_OFFSET:
                printf(EBML_WARNING "deprecated ID 0x%x on position %ld\n", code, get_current_byte(file));
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case EBML_VOID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_CRC32:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == EBML_MAX_ID_LENGTH) {
                    printf(EBML_ERROR "Unknown ID 0x%x, skipping segment track entry block\n", code);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment_tracks(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Tracks ids*/
            case EBML_SEGMENT_TRACK_ENTRY:
                parse_segment_track_entry(file);
                EBML_SWITCH_BREAK(code, code_len);

                /* Misc ids */
            case EBML_VOID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_CRC32:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == EBML_MAX_ID_LENGTH) {
                    printf(EBML_ERROR "Unknown ID 0x%x, skipping segment tracks block\n", code);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment ids */
            case EBML_SEGMENT_SEEK_HEAD:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_INFO:
                parse_segment_info(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACKS:
                parse_segment_tracks(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CUES:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_ATTACHMENTS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CHAPTERS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TAGS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case EBML_VOID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_CRC32:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == EBML_MAX_ID_LENGTH) {
                    printf (EBML_ERROR "Unknown ID 0x%x, skipping segment block\n", code);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse(FILE* file) {
    int code = 0, code_len = 0;
    while (!feof(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Header ids*/
            case EBML_EBML_HEADER:
                parse_ebml(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_HEADER:
                parse_segment(file);
                EBML_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case EBML_VOID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_CRC32:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == EBML_MAX_ID_LENGTH) {
                    printf (EBML_ERROR "Unknown ID 0x%x, skipping file parsing\n", code);
                    return;
                }
                break;
        }
    }
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        printf("======================= New video file: %s =======================\n", argv[i]);
        FILE *file;
        file = fopen(argv[i], "r");
        parse(file);
        printf("\n\n\n\n");
    }
}