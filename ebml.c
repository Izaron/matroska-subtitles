#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ebml.h"

void skip_bytes(FILE* file, unsigned long n) {
    fseek(file, n, SEEK_CUR);
}

void set_bytes(FILE* file, unsigned long n) {
    fseek(file, n, SEEK_SET);
}

unsigned long get_current_byte(FILE* file) {
    return (unsigned long) ftell(file);
}

unsigned char* read_bytes(FILE* file, unsigned  long n) {
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
                    printf (EBML_ERROR "Unknown element 0x%x at position %ld, skipping EBML block\n", code,
                            get_current_byte(file) - EBML_MAX_ID_LENGTH);
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
                    printf (EBML_ERROR "Unknown element 0x%x at position %ld, skipping segment info block\n", code,
                            get_current_byte(file) - EBML_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment_cluster_block_group_block(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);
    long track_number = read_vint_length(file);     // track number is length, not int
    if (track_number < 4) {
        set_bytes(file, pos + len);
        return;
    }

    long timecode = read_byte(file);
    timecode <<= 8; timecode += read_byte(file);

    read_byte(file);    // skip one byte

    read_bytes(file, pos + len - get_current_byte(file));
    //printf("%s\n\n", read_bytes(file, pos + len - get_current_byte(file)));
}

void parse_segment_cluster_block_group(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster block group ids */
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK:
                parse_segment_cluster_block_group_block(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_VIRTUAL:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_ADDITIONS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_DURATION:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_PRIORITY:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_CODEC_STATE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_DISCARD_PADDING:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_SLICES:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_FRAME:
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
                    printf (EBML_ERROR "Unknown element 0x%x at position %ld, skipping segment cluster block group\n", code,
                            get_current_byte(file) - EBML_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment_cluster(FILE* file) {
    long len = read_vint_length(file);
    long pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster ids */
            case EBML_SEGMENT_CLUSTER_TIMECODE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_SILENT_TRACKS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_POSITION:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_PREV_SIZE:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_SIMPLE_BLOCK:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP:
                parse_segment_cluster_block_group(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_ENCRYPTED_BLOCK:
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
                    printf (EBML_ERROR "Unknown element 0x%x at position %ld, skipping segment cluster block\n", code,
                            get_current_byte(file) - EBML_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* get_track_entry_type_description(unsigned long type) {
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
                printf(EBML_WARNING "Deprecated element 0x%x at position %ld\n", code,
                       get_current_byte(file) - 3); // minus length of the ID
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_OFFSET:
                printf(EBML_WARNING "Deprecated element 0x%x at position %ld\n", code,
                       get_current_byte(file) - 2); // minus length of the ID
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);

            /* DivX trick track extenstions */
            case EBML_SEGMENT_TRACK_TRICK_TRACK_UID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRICK_TRACK_SEGMENT_UID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRICK_TRACK_FLAG:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRICK_MASTER_TRACK_UID:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID:
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
                    printf(EBML_ERROR "Unknown element 0x%x at position %ld, skipping segment track entry block\n", code,
                           get_current_byte(file) - EBML_MAX_ID_LENGTH);
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
                    printf(EBML_ERROR "Unknown element 0x%x at position %ld, skipping segment tracks block\n", code,
                           get_current_byte(file) - EBML_MAX_ID_LENGTH);
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
                //read_vint_block_skip(file);
                parse_segment_cluster(file);
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
                    printf (EBML_ERROR "Unknown element 0x%x at position %ld, skipping segment block\n", code,
                            get_current_byte(file) - EBML_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse(FILE* file) {
    int code = 0, code_len = 0;
    if (file == NULL) {
        printf(EBML_ERROR "can't open file. Is it exists?");
        return;
    }
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
                    printf (EBML_ERROR "Unknown element 0x%x at position %ld, skipping file parsing\n", code,
                            get_current_byte(file) - EBML_MAX_ID_LENGTH);
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