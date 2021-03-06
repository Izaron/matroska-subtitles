#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#ifdef _WIN32
#include <sys/stat.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <limits.h>
#include "matroska.h"

struct matroska_ctx* mkv_ctx;

void skip_bytes(FILE* file, matroska_int n) {
#ifdef _WIN32
    _fseeki64(file, n, SEEK_CUR);
#else
    fseeko(file, n, SEEK_CUR);
#endif
}

void set_bytes(FILE* file, matroska_int n) {
#ifdef _WIN32
    _fseeki64(file, n, SEEK_SET);
#else
    fseeko(file, n, SEEK_SET);
#endif
}

void set_byte_at_the_end(FILE* file) {
#ifdef _WIN32
    _fseeki64(file, 0, SEEK_END);
#else
    fseeko(file, 0, SEEK_END);
#endif
}

matroska_int get_current_byte(FILE* file) {
#ifdef _WIN32
    return (matroska_int)_ftelli64(file);
#else
    return (matroska_int)ftello(file);
#endif
}

matroska_byte* read_bytes(FILE* file, matroska_int n) {
    matroska_byte* buffer = malloc((size_t)(sizeof(matroska_byte) * n));
    fread(buffer, 1, (size_t)n, file);
    return buffer;
}

char* read_bytes_signed(FILE* file, matroska_int n) {
    char* buffer = malloc((size_t)(sizeof(matroska_byte) * (n + 1)));
    fread(buffer, 1, (size_t)n, file);
    buffer[n] = 0;
    return buffer;
}

matroska_byte read_byte(FILE* file) {
    return (matroska_byte)fgetc(file);
}

matroska_int read_vint_length(FILE* file) {
    matroska_byte ch = read_byte(file);
    int cnt = 8 - __lzcnt16(ch);
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

char* read_vint_block_signed(FILE* file) {
    matroska_int len = read_vint_length(file);
    return read_bytes_signed(file, len);
}

matroska_int read_vint_block_int(FILE* file) {
    matroska_int len = read_vint_length(file);
    matroska_byte* s = read_bytes(file, len);

    matroska_int res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 8;
        res += s[i];
    }

    free(s);
    return res;
}

char* read_vint_block_string(FILE* file) {
    return read_vint_block_signed(file);
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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping EBML block\n", code,
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
                printf("Timecode scale: %lld\n", read_vint_block_int(file));
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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping segment info block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* generate_timestamp_utf8(matroska_int milliseconds) {
    matroska_int millis = milliseconds % 1000;
    milliseconds /= 1000;
    matroska_int seconds = milliseconds % 60;
    milliseconds /= 60;
    matroska_int minutes = milliseconds % 60;
    milliseconds /= 60;
    matroska_int hours = milliseconds;

    char* buf = malloc(sizeof(char) * 15);
    sprintf(buf, "%02lld:%02lld:%02lld,%03lld", hours, minutes, seconds, millis);
    return buf;
}

char* generate_timestamp_ass_ssa(matroska_int milliseconds) {
    matroska_int millis = (milliseconds % 1000) / 10;
    milliseconds /= 1000;
    matroska_int seconds = milliseconds % 60;
    milliseconds /= 60;
    matroska_int minutes = milliseconds % 60;
    milliseconds /= 60;
    matroska_int hours = milliseconds;

    char* buf = malloc(sizeof(char) * 15);
    sprintf(buf, "%lld:%02lld:%02lld.%02lld", hours, minutes, seconds, millis);
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
    char* message = read_bytes_signed(file, size);

    struct matroska_sub_sentence* sentence = malloc(sizeof(struct matroska_sub_sentence));
    sentence->text = message;
    sentence->text_size = size;
    sentence->time_start = timecode + cluster_timecode;

    struct matroska_sub_track* track = mkv_ctx->sub_tracks[sub_track_index];
    //track->sentences = realloc(track->sentences, sizeof(struct matroska_sub_track*) * (track->sentence_count + 1));
    track->sentences[track->sentence_count] = sentence;
    track->sentence_count++;

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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping segment cluster block group\n", code,
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

    free(sentence_list);
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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping segment cluster block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }

    activity_progress((int)(100 * get_current_byte(file) / mkv_ctx->file_size));
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

enum matroska_track_subtitle_codec_id get_track_subtitle_codec_id(char* codec_id) {
    for (int i = MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8; i <= MATROSKA_TRACK_SUBTITLE_CODEC_ID_KATE; i++)
        if (strcmp(codec_id, matroska_track_text_subtitle_id_strings[i]) == 0)
            return (enum matroska_track_subtitle_codec_id) i;
    return (enum matroska_track_subtitle_codec_id) 0;
}

void parse_segment_track_entry(FILE* file) {
    printf("\n==== Track entry ====\n");

    matroska_int len = read_vint_length(file);
    matroska_int pos = get_current_byte(file);

    matroska_int track_number = 0;
    enum matroska_track_entry_type track_type = MATROSKA_TRACK_TYPE_VIDEO;
    char* lang = strdup("eng");
    char* header = NULL;
    char* codec_id_string = NULL;
    enum matroska_track_subtitle_codec_id codec_id = MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8;
    char* tmp;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Track entry ids*/
            case MATROSKA_SEGMENT_TRACK_TRACK_NUMBER:
                track_number = read_vint_block_int(file);
                printf("Track number: %lld\n", track_number);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_UID:
                printf("UID: %llu\n", read_vint_block_int(file));
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
                write(1, "Name: ", strlen("Name: "));
                tmp = read_vint_block_string(file);
                write(1, tmp, strlen(tmp));
                write(1, "\n", 1);
                //printf("Name: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_LANGUAGE:
                lang = read_vint_block_string(file);
                printf("Language: %s\n", lang);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_ID:
                codec_id_string = read_vint_block_string(file);
                codec_id = get_track_subtitle_codec_id(codec_id_string);
                printf("Codec ID: %s\n", codec_id_string);
                free(codec_id_string);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_PRIVATE:
                if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE) {
                    header = read_vint_block_string(file);
                }
                else {
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
                printf(MATROSKA_WARNING "Deprecated element 0x%x at position %lld\n", code,
                       get_current_byte(file) - 3); // minus length of the ID
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_OFFSET:
                printf(MATROSKA_WARNING "Deprecated element 0x%x at position %lld\n", code,
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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping segment track entry block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }

    if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE) {
        struct matroska_sub_track* sub_track = malloc(sizeof(struct matroska_sub_track));
        sub_track->header = header;
        sub_track->lang = lang;
        sub_track->track_number = track_number;
        sub_track->lang_index = 0;
        sub_track->codec_id = codec_id;
        sub_track->sentence_count = 0;

        for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
            if (strcmp((const char *)mkv_ctx->sub_tracks[i]->lang, (const char *)lang) == 0)
                sub_track->lang_index++;

        mkv_ctx->sub_tracks;
        //mkv_ctx->sub_tracks = realloc(mkv_ctx->sub_tracks, sizeof(struct matroska_sub_track*) * (mkv_ctx->sub_tracks_count + 1));
        mkv_ctx->sub_tracks[mkv_ctx->sub_tracks_count] = sub_track;
        mkv_ctx->sub_tracks_count++;
    }
    else {
        free(lang);
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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping segment tracks block\n", code,
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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping segment block\n", code,
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
        sprintf(buf, "%s_%s.%s", mkv_ctx->filename, track->lang, matroska_track_text_subtitle_id_extensions[track->codec_id]);
    else
        sprintf(buf, "%s_%s_%lld.%s", mkv_ctx->filename, track->lang, track->lang_index,
                matroska_track_text_subtitle_id_extensions[track->codec_id]);
    write(1, buf, strlen(buf));
    write(1, "\n", 1);
    return buf;
}

char* ass_ssa_sentence_erase_read_order(char* text) {
    // crop text after second ','
    int cnt = 0;
    int index = 0;
    while (cnt < 2) {
        if (text[index] == ',')
            cnt++;
        index++;
    }
    int len = strlen(text) - index;
    char* buf = malloc(sizeof(char) * (len + 1));
    memcpy(buf, &text[index], len);
    buf[len] = '\0';
    return buf;
}

void save_sub_track(struct matroska_sub_track* track) {
    char* filename = generate_filename_from_track(track);
    int desc;
#ifdef WIN32
    desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IREAD | S_IWRITE);
#else
    desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IWUSR | S_IRUSR);
#endif
    free(filename);

    if (track->header != NULL)
        write(desc, track->header, strlen(track->header));

    for (int i = 0; i < track->sentence_count; i++) {
        struct matroska_sub_sentence* sentence = track->sentences[i];

        if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8) {
            char number[9];
            sprintf(number, "%d", i + 1);

            char *timestamp_start = generate_timestamp_utf8(sentence->time_start);
            matroska_int time_end = sentence->time_end;
            if (i + 1 < track->sentence_count)
                time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
            char *timestamp_end = generate_timestamp_utf8(time_end);

            write(desc, number, strlen(number));
            write(desc, "\n", 1);
            write(desc, timestamp_start, strlen(timestamp_start));
            write(desc, " --> ", 5);
            write(desc, timestamp_end, strlen(timestamp_start));
            write(desc, "\n", 1);
            write(desc, sentence->text, sentence->text_size);
            write(desc, "\n\n", 2);

            free(timestamp_start);
            free(timestamp_end);
        }
        else if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_ASS || track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_SSA) {
            char *timestamp_start = generate_timestamp_ass_ssa(sentence->time_start);
            matroska_int time_end = sentence->time_end;
            if (i + 1 < track->sentence_count)
                time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
            char *timestamp_end = generate_timestamp_ass_ssa(time_end);

            write(desc, "Dialogue: Marked=0,", strlen("Dialogue: Marked=0,"));
            write(desc, timestamp_start, strlen(timestamp_start));
            write(desc, ",", 1);
            write(desc, timestamp_end, strlen(timestamp_start));
            write(desc, ",", 1);
            char* text = ass_ssa_sentence_erase_read_order(sentence->text);
            write(desc, text, strlen(text));
            write(desc, "\n", 1);

            free(timestamp_start);
            free(timestamp_end);
        }
    }
}

void free_sub_track(struct matroska_sub_track* track) {
    if (track->header != NULL)
        free(track->header);
    if (track->lang != NULL)
        free(track->lang);
    for (int i = 0; i < track->sentence_count; i++) {
        struct matroska_sub_sentence* sentence = track->sentences[i];
        free(sentence->text);
        free(sentence);
    }
    free(track);
}

void save_all_sub_tracks() {
    for (int i = 0; i < mkv_ctx->sub_tracks_count; i++) {
        save_sub_track(mkv_ctx->sub_tracks[i]);
        free_sub_track(mkv_ctx->sub_tracks[i]);
    }
}

void parse(FILE* file) {
    int code = 0, code_len = 0;
    if (file == NULL) {
        printf(MATROSKA_ERROR "can't open file. Is it exists?");
        return;
    }

    // Get a file's size
    // I guess it's bad for streaming, but Matroska have not any streams
    set_byte_at_the_end(file);
    mkv_ctx->file_size = get_current_byte(file);
    set_bytes(file, 0);

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
                    printf(MATROSKA_ERROR "Unknown element 0x%x at position %lld, skipping file parsing\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    return;
                }
                break;
        }
    }
    fclose(file);

    activity_progress(100);
    printf("\n");
    save_all_sub_tracks();
}

void activity_progress(int percentage)
{
    printf("\r%3d%%  | Streaming...", percentage);
    fflush(stdout);
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        printf("======================= New video file: %s =======================\n", argv[i]);
        FILE *file;
        mkv_ctx = malloc(sizeof(struct matroska_ctx));
        mkv_ctx->sub_tracks_count = 0;
        mkv_ctx->filename = argv[i];
        file = fopen(argv[i], "rb");
        parse(file);
        free(mkv_ctx);
    }
}
