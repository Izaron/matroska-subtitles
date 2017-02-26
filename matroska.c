#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include "matroska.h"

struct matroska_ctx* mkv_ctx;

void skip_bytes(FILE* file, matroska_int n) {
    fseek(file, n, SEEK_CUR);
}

void set_bytes(FILE* file, matroska_int n) {
    fseek(file, n, SEEK_SET);
}

matroska_int get_current_byte(FILE* file) {
    return (matroska_int) ftell(file);
}

matroska_byte* read_bytes(FILE* file, matroska_int n) {
    matroska_byte* buffer = malloc(sizeof(matroska_byte) * n);
    fread(buffer, 1, n, file);
    return buffer;
}

matroska_byte read_byte(FILE* file) {
    return (matroska_byte) fgetc(file);
}

matroska_int read_vint_length(FILE* file) {
    matroska_byte ch = read_byte(file);
    int cnt = __builtin_clz(ch) + 1 - 24;
    ch ^= (1 << (8 - cnt));
    matroska_int ret = ch;
    for (int i = 1; i < cnt; i++) {
        ret <<= 8;
        ret += read_byte(file);
    }
    return ret;
}

matroska_byte* read_vint_block(FILE* file) {
    matroska_int len = read_vint_length(file);
    return read_bytes(file, len);
}

matroska_int read_vint_block_int(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_byte* s = read_bytes(file, len);

    matroska_int res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 8;
        res += s[i];
    }

    return res;
}

matroska_byte* read_vint_block_string(FILE* file) {
    return read_vint_block(file);
}

void read_vint_block_skip(FILE* file) {
    matroska_int len = read_vint_length(file);
    skip_bytes(file, len);
}

void parse_ebml(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* EBML ids */
            case MATROSKA_EBML_VERSION:
                read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_EBML_READ_VERSION:
                read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_EBML_MAX_ID_LENGTH:
                read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_EBML_MAX_SIZE_LENGTH:
                read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_EBML_DOC_TYPE:
                printf("Document type: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_EBML_DOC_TYPE_VERSION:
                read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_EBML_DOC_TYPE_READ_VERSION:
                read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf (MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping EBML block\n", code,
                            get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment_info(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment info ids */
            case MATROSKA_SEGMENT_INFO_SEGMENT_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_SEGMENT_FILENAME:
                printf("Filename: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_PREV_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_PREV_FILENAME:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_NEXT_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_NEXT_FILENAME:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_SEGMENT_FAMILY:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_CHAPTER_TRANSLATE:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_TIMECODE_SCALE:
                printf("Timecode scale: %ld\n", read_vint_block_int(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_DURATION:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_DATE_UTC:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_TITLE:
                printf("Title: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_MUXING_APP:
                printf("Muxing app: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_WRITING_APP:
                printf("Writing app: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf (MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping segment info block\n", code,
                            get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* generate_timestamp(matroska_int milliseconds) {
    matroska_int millis = milliseconds % 1000;
    milliseconds /= 1000;
    matroska_int seconds = milliseconds % 60;
    milliseconds /= 60;
    matroska_int minutes = milliseconds % 60;
    milliseconds /= 60;
    matroska_int hours = milliseconds;

    char* buf = malloc(sizeof(char) * 15);
    sprintf(buf, "%02ld:%02ld:%02ld,%03ld", hours, minutes, seconds, millis);
    return buf;
}

int find_sub_track_index(matroska_int track_number) {
    for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
        if (mkv_ctx->sub_tracks[i]->track_number == track_number)
            return i;
    return -1;
}

struct matroska_sub_sentence* parse_segment_cluster_block_group_block(FILE* file, matroska_int cluster_timecode) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);
    matroska_int track_number = read_vint_length(file);     // track number is length, not int

    int sub_track_index = find_sub_track_index(track_number);
    if (sub_track_index == -1) {
        set_bytes(file, pos + len);
        return NULL;
    }

    matroska_int timecode = read_byte(file);
    timecode <<= 8; timecode += read_byte(file);

    read_byte(file);    // skip one byte

    matroska_int size = pos + len - get_current_byte(file);
    matroska_byte * message = read_bytes(file, size);

    struct matroska_sub_sentence* sentence = malloc(sizeof(struct matroska_sub_sentence));
    sentence->text = message;
    sentence->text_size = size;
    sentence->time_start = timecode + cluster_timecode;

    struct matroska_sub_track* track = mkv_ctx->sub_tracks[sub_track_index];
    track->sentences = realloc(track->sentences, sizeof(struct matroska_sub_track*) * (track->sentence_count + 1));
    track->sentences[track->sentence_count] = sentence;
    track->sentence_count++;

    /*char buffer[15];
    sprintf(buffer, "sub_.txt");

    char* start_time = generate_timestamp(timecode + cluster_timecode);*/

    /*int desc = open(buffer, O_CREAT | O_WRONLY | O_APPEND, S_IWUSR | S_IRUSR);
    write(desc, start_time, strlen(start_time));
    write(desc, "\n", 1);
    write(desc, message, size);
    write(desc, "\n\n", 2);
    close(desc);*/

    return sentence;
}

void parse_segment_cluster_block_group(FILE* file, matroska_int cluster_timecode) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    matroska_int block_duration = ULONG_MAX;
    struct matroska_sub_sentence* new_sentence;
    struct matroska_sub_sentence** sentence_list = NULL;
    int sentence_count = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster block group ids */
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK:
                new_sentence = parse_segment_cluster_block_group_block(file, cluster_timecode);
                if (new_sentence != NULL) {
                    sentence_list = realloc(sentence_list, sizeof(struct matroska_sub_track*) * (sentence_count + 1));
                    sentence_list[sentence_count] = new_sentence;
                    sentence_count++;
                }
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_VIRTUAL:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_ADDITIONS:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_DURATION:
                block_duration = read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_PRIORITY:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_CODEC_STATE:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_DISCARD_PADDING:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_SLICES:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_FRAME:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf (MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping segment cluster block group\n", code,
                            get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }

    for (int i = 0; i < sentence_count; i++) {
        // When BlockDuration is not written, the value is assumed to be the difference
        // between the timestamp of this Block and the timestamp of the next Block in "display" order
        if (block_duration == ULONG_MAX)
            sentence_list[i]->time_end = ULONG_MAX;
        else
            sentence_list[i]->time_end = sentence_list[i]->time_start + block_duration;
    }
}

void parse_segment_cluster(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    matroska_int timecode = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster ids */
            case MATROSKA_SEGMENT_CLUSTER_TIMECODE:
                timecode = read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_SILENT_TRACKS:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_POSITION:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_PREV_SIZE:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_SIMPLE_BLOCK:
                // Same as Block inside the Block Group, but we can't save subs in this structure
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP:
                parse_segment_cluster_block_group(file, timecode);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER_ENCRYPTED_BLOCK:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf (MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping segment cluster block\n", code,
                            get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* get_track_entry_type_description(enum matroska_track_entry_type type) {
    switch (type) {
        case MATROSKA_TRACK_TYPE_VIDEO:
            return "video";
        case MATROSKA_TRACK_TYPE_AUDIO:
            return "audio";
        case MATROSKA_TRACK_TYPE_COMPLEX:
            return "complex";
        case MATROSKA_TRACK_TYPE_LOGO:
            return "logo";
        case MATROSKA_TRACK_TYPE_SUBTITLE:
            return "subtitle";
        case MATROSKA_TRACK_TYPE_BUTTONS:
            return "buttons";
        case MATROSKA_TRACK_TYPE_CONTROL:
            return "control";
        default:
            return NULL;
    }
}

void parse_segment_track_entry(FILE* file) {
    printf("\n==== Track entry ====\n");

    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    matroska_int track_number = 0;
    enum matroska_track_entry_type track_type = MATROSKA_TRACK_TYPE_VIDEO;
    matroska_byte* lang = (matroska_byte *) "eng";
    matroska_byte* header = NULL;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Track entry ids*/
            case MATROSKA_SEGMENT_TRACK_TRACK_NUMBER:
                track_number = read_vint_block_int(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_UID:
                printf("UID: %lu\n", read_vint_block_int(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_TYPE:
                track_type = (enum matroska_track_entry_type) read_vint_block_int(file);
                printf("Type: %s\n", get_track_entry_type_description(track_type));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_FLAG_ENABLED:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_FLAG_DEFAULT:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_FLAG_FORCED:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_FLAG_LACING:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_MIN_CACHE:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_MAX_CACHE:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_DEFAULT_DURATION:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_DEFAULT_DECODED_FIELD_DURATION:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_MAX_BLOCK_ADDITION_ID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_NAME:
                printf("Name: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_LANGUAGE:
                lang = read_vint_block_string(file);
                printf("Language: %s\n", lang);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_ID:
                printf("Codec ID: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_PRIVATE:
                if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE) {
                    header = read_vint_block_string(file);
                } else {
                    read_vint_block_skip(file);
                }
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_NAME:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_ATTACHMENT_LINK:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_DECODE_ALL:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_OVERLAY:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_DELAY:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_SEEK_PRE_ROLL:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_TRANSLATE:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_VIDEO:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_AUDIO:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_OPERATION:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CONTENT_ENCODINGS:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Deprecated IDs */
            case MATROSKA_SEGMENT_TRACK_TRACK_TIMECODE_SCALE:
                printf(MATROSKA_WARNING "Deprecated element 0x%x at position %ld\n", code,
                       get_current_byte(file) - 3); // minus length of the ID
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_OFFSET:
                printf(MATROSKA_WARNING "Deprecated element 0x%x at position %ld\n", code,
                       get_current_byte(file) - 2); // minus length of the ID
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* DivX trick track extenstions */
            case MATROSKA_SEGMENT_TRACK_TRICK_TRACK_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRICK_TRACK_SEGMENT_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRICK_TRACK_FLAG:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRICK_MASTER_TRACK_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping segment track entry block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }

    if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE) {
        struct matroska_sub_track* sub_track = malloc(sizeof(struct matroska_sub_track));
        sub_track->track_number = track_number;
        sub_track->lang = lang;
        sub_track->lang_index = 0;
        sub_track->header = header;
        sub_track->sentence_count = 0;

        for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
            if (strcmp((const char *) mkv_ctx->sub_tracks[i]->lang, (const char *) lang) == 0)
                sub_track->lang_index++;

        mkv_ctx->sub_tracks;
        mkv_ctx->sub_tracks = realloc(mkv_ctx->sub_tracks, sizeof(struct matroska_sub_track*) * (mkv_ctx->sub_tracks_count + 1));
        mkv_ctx->sub_tracks[mkv_ctx->sub_tracks_count] = sub_track;
        mkv_ctx->sub_tracks_count++;
    }
}

void parse_segment_tracks(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Tracks ids*/
            case MATROSKA_SEGMENT_TRACK_ENTRY:
                parse_segment_track_entry(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping segment tracks block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment ids */
            case MATROSKA_SEGMENT_SEEK_HEAD:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO:
                parse_segment_info(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER:
                //read_vint_block_skip(file);
                parse_segment_cluster(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACKS:
                parse_segment_tracks(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CUES:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_ATTACHMENTS:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CHAPTERS:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TAGS:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf (MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping segment block\n", code,
                            get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* generate_filename_from_track(struct matroska_sub_track* track) {
    char* buf = malloc(sizeof(char) * 200);
    if (track->lang_index == 0)
        sprintf(buf, "%s_%s.srt", mkv_ctx->filename, track->lang);
    else
        sprintf(buf, "%s_%s_%ld.srt", mkv_ctx->filename, track->lang, track->lang_index);
    write(1, buf, strlen(buf));
    write(1, "\n", 1);
    return buf;
}

void save_sub_track(struct matroska_sub_track* track) {
    int desc = open(generate_filename_from_track(track), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IWUSR | S_IRUSR);

    if (track->header != NULL)
        write(desc, track->header, strlen(track->header));

    for (int i = 0; i < track->sentence_count; i++) {
        struct matroska_sub_sentence* sentence = track->sentences[i];

        char number[9];
        sprintf(number, "%d", i + 1);

        char* timestamp_start = generate_timestamp(sentence->time_start);
        matroska_int time_end = sentence->time_end;
        if (i + 1 < track->sentence_count)
            time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
        char* timestamp_end = generate_timestamp(time_end);

        write(desc, number, strlen(number));
        write(desc, "\n", 1);
        write(desc, timestamp_start, strlen(timestamp_start));
        write(desc, " --> ", 5);
        write(desc, timestamp_end, strlen(timestamp_start));
        write(desc, "\n", 1);
        write(desc, sentence->text, sentence->text_size);
        write(desc, "\n\n", 2);
    }
}

void save_all_sub_tracks() {
    for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
        save_sub_track(mkv_ctx->sub_tracks[i]);
}

void parse(FILE* file) {
    int code = 0, code_len = 0;
    if (file == NULL) {
        printf(MATROSKA_ERROR "can't open file. Is it exists?");
        return;
    }
    while (!feof(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Header ids*/
            case MATROSKA_EBML_HEADER:
                parse_ebml(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_HEADER:
                parse_segment(file);
                MATROSKA_SWITCH_BREAK(code, code_len);

            /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    printf (MATROSKA_ERROR "Unknown element 0x%x at position %ld, skipping file parsing\n", code,
                            get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    return;
                }
                break;
        }
    }
    fclose(file);

    save_all_sub_tracks();
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        printf("======================= New video file: %s =======================\n", argv[i]);
        FILE *file;
        mkv_ctx = malloc(sizeof(mkv_ctx));
        mkv_ctx->sub_tracks_count = 0;
        mkv_ctx->filename = argv[i];

        file = fopen(argv[i], "r");
        parse(file);
        printf("\n\n\n\n");
    }
}