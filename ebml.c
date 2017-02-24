#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ebml.h"

int is_track_subtitle[EBML_MAX_TRACK_NUMBER];

void skip_bytes(FILE* file, EBML_int n) {
    fseek(file, n, SEEK_CUR);
}

void set_bytes(FILE* file, EBML_int n) {
    fseek(file, n, SEEK_SET);
}

EBML_int get_current_byte(FILE* file) {
    return (EBML_int) ftell(file);
}

EBML_byte* read_bytes(FILE* file, EBML_int n) {
    EBML_byte* buffer = malloc(sizeof(EBML_byte) * n);
    fread(buffer, 1, n, file);
    return buffer;
}

EBML_byte read_byte(FILE* file) {
    EBML_byte ch;
    fread(&ch, 1, 1, file);
    return ch;
}

EBML_int read_vint_length(FILE* file) {
    EBML_byte ch = read_byte(file);
    int cnt = 1;
    for (int i = 7; i >= 0; i--) {
        if ((ch & (1 << i)) != 0) {
            ch ^= (1 << i);
            break;
        } else
            cnt++;
    }
    EBML_int ret = ch;
    for (int i = 1; i < cnt; i++) {
        ret <<= 8;
        ret += read_byte(file);
    }
    return ret;
}

EBML_byte* read_vint_block(FILE* file) {
    EBML_int len = read_vint_length(file);
    return read_bytes(file, len);
}

EBML_int read_vint_block_int(FILE* file) {
    EBML_int len = read_vint_length(file);
    EBML_byte* s = read_bytes(file, len);

    EBML_int res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 8;
        res += s[i];
    }

    return res;
}

EBML_byte* read_vint_block_string(FILE* file) {
    return read_vint_block(file);
}

void read_vint_block_skip(FILE* file) {
    EBML_int len = read_vint_length(file);
    skip_bytes(file, len);
}

void parse_ebml(FILE* file) {
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

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
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

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

void parse_segment_cluster_block_group_block(FILE* file, EBML_int cluster_timecode) {
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);
    EBML_int track_number = read_vint_length(file);     // track number is length, not int

    if (!is_track_subtitle[track_number]) {
        set_bytes(file, pos + len);
        return;
    }

    EBML_int timecode = read_byte(file);
    timecode <<= 8; timecode += read_byte(file);

    read_byte(file);    // skip one byte

    printf("Pos: %ld\n", get_current_byte(file));
    printf("Time code: %ld\n", timecode + cluster_timecode);
    printf("%s\n\n", read_bytes(file, pos + len - get_current_byte(file)));
}

void parse_segment_cluster_block_group(FILE* file, EBML_int cluster_timecode) {
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

    EBML_int block_duration = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster block group ids */
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK:
                parse_segment_cluster_block_group_block(file, cluster_timecode);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_VIRTUAL:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_ADDITIONS:
                read_vint_block_skip(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_DURATION:
                block_duration = read_vint_block_int(file);
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
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

    EBML_int timecode = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster ids */
            case EBML_SEGMENT_CLUSTER_TIMECODE:
                timecode = read_vint_block_int(file);
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
                parse_segment_cluster_block_group(file, timecode);
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

char* get_track_entry_type_description(EBML_int type) {
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

    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

    EBML_int track_number = 0;
    EBML_int track_type = 0;
    EBML_byte* lang = "";

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Track entry ids*/
            case EBML_SEGMENT_TRACK_TRACK_NUMBER:
                track_number = read_vint_block_int(file);
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_UID:
                printf("UID: %lu\n", read_vint_block_int(file));
                EBML_SWITCH_BREAK(code, code_len);
            case EBML_SEGMENT_TRACK_TRACK_TYPE:
                track_type = read_vint_block_int(file);
                printf("Type: %s\n", get_track_entry_type_description(track_type));
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
                lang = read_vint_block_string(file);
                printf("Language: %s\n", lang);
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

    if (track_type == EBML_TRACK_TYPE_CODE_SUBTITLE) {
        is_track_subtitle[track_number] = 1;
    }
}

void parse_segment_tracks(FILE* file) {
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

    memset(is_track_subtitle, 0, sizeof(is_track_subtitle));

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
    EBML_int len = read_vint_length(file);
    EBML_int pos = get_current_byte(file);

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
    fclose(file);
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