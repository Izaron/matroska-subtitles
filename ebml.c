#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include "ebml.h"

struct ebml_sub_track* sub_tracks[EBML_MAX_TRACKS];

void skip_bytes(FILE* file, ebml_int n) {
    fseek(file, n, SEEK_CUR);
}

void set_bytes(FILE* file, ebml_int n) {
    fseek(file, n, SEEK_SET);
}

ebml_int get_current_byte(FILE* file) {
    return (ebml_int) ftell(file);
}

ebml_byte* read_bytes(FILE* file, ebml_int n) {
    ebml_byte* buffer = malloc(sizeof(ebml_byte) * n);
    fread(buffer, 1, n, file);
    return buffer;
}

ebml_byte read_byte(FILE* file) {
    ebml_byte ch;
    fread(&ch, 1, 1, file);
    return ch;
}

ebml_int read_vint_length(FILE* file) {
    ebml_byte ch = read_byte(file);
    int cnt = 1;
    for (int i = 7; i >= 0; i--) {
        if ((ch & (1 << i)) != 0) {
            ch ^= (1 << i);
            break;
        } else
            cnt++;
    }
    ebml_int ret = ch;
    for (int i = 1; i < cnt; i++) {
        ret <<= 8;
        ret += read_byte(file);
    }
    return ret;
}

ebml_byte* read_vint_block(FILE* file) {
    ebml_int len = read_vint_length(file);
    return read_bytes(file, len);
}

ebml_int read_vint_block_int(FILE* file) {
    ebml_int len = read_vint_length(file);
    ebml_byte* s = read_bytes(file, len);

    ebml_int res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 8;
        res += s[i];
    }

    return res;
}

ebml_byte* read_vint_block_string(FILE* file) {
    return read_vint_block(file);
}

void read_vint_block_skip(FILE* file) {
    ebml_int len = read_vint_length(file);
    skip_bytes(file, len);
}

void parse_ebml(FILE* file) {
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

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
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

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

char* generate_timestamp(ebml_int milliseconds) {
    ebml_int millis = milliseconds % 1000;
    milliseconds /= 1000;
    ebml_int seconds = milliseconds % 60;
    milliseconds /= 60;
    ebml_int minutes = milliseconds % 60;
    milliseconds /= 60;
    ebml_int hours = milliseconds;

    char* buf = malloc(sizeof(char) * 15);
    sprintf(buf, "%02ld:%02ld:%02ld,%03ld", hours, minutes, seconds, millis);
    return buf;
}

int find_sub_track_index(ebml_int track_number) {
    int index = 0;
    while (sub_tracks[index] != NULL) {
        if (sub_tracks[index]->track_number == track_number)
            return index;
        index++;
    }
    return -1;
}

struct ebml_sub_sentence* parse_segment_cluster_block_group_block(FILE* file, ebml_int cluster_timecode) {
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);
    ebml_int track_number = read_vint_length(file);     // track number is length, not int

    int sub_track_index = find_sub_track_index(track_number);
    if (sub_track_index == -1) {
        set_bytes(file, pos + len);
        return NULL;
    }

    ebml_int timecode = read_byte(file);
    timecode <<= 8; timecode += read_byte(file);

    read_byte(file);    // skip one byte

    ebml_int size = pos + len - get_current_byte(file);
    ebml_byte * message = read_bytes(file, size);

    struct ebml_sub_sentence* sentence = malloc(sizeof(struct ebml_sub_sentence));
    sentence->text = message;
    sentence->text_size = size;
    sentence->time_start = timecode + cluster_timecode;

    struct ebml_sub_track* track = sub_tracks[sub_track_index];
    track->sentences = realloc(track->sentences, sizeof(struct ebml_sub_track*) * (track->sentence_count + 1));
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

void parse_segment_cluster_block_group(FILE* file, ebml_int cluster_timecode) {
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

    ebml_int block_duration = ULONG_MAX;
    struct ebml_sub_sentence* new_sentence;
    struct ebml_sub_sentence** sentence_list = NULL;
    int sentence_count = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster block group ids */
            case EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK:
                new_sentence = parse_segment_cluster_block_group_block(file, cluster_timecode);
                if (new_sentence != NULL) {
                    sentence_list = realloc(sentence_list, sizeof(struct ebml_sub_track*) * (sentence_count + 1));
                    sentence_list[sentence_count] = new_sentence;
                    sentence_count++;
                }
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
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

    ebml_int timecode = 0;

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
                // Same as Block inside the Block Group, but we can't save subs in this structure
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

char* get_track_entry_type_description(ebml_int type) {
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

    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

    ebml_int track_number = 0;
    ebml_int track_type = 0;
    ebml_byte* lang = (ebml_byte *) "eng";

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
                read_vint_block_skip(file);
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
        int index = 0;
        while (sub_tracks[index] != NULL)
            index++;
        sub_tracks[index] = malloc(sizeof(struct ebml_sub_track));
        sub_tracks[index]->track_number = track_number;
        sub_tracks[index]->lang = lang;
        sub_tracks[index]->lang_index = 0;
        sub_tracks[index]->sentence_count = 0;
        for (int i = 0; i < index; i++)
            if (strcmp((const char *) sub_tracks[i]->lang, (const char *) lang) == 0)
                sub_tracks[index]->lang_index++;
    }
}

void parse_segment_tracks(FILE* file) {
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

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
    ebml_int len = read_vint_length(file);
    ebml_int pos = get_current_byte(file);

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

void save_sub_track(struct ebml_sub_track* track) {
    for (int i = 0; i < track->sentence_count; i++) {
        struct ebml_sub_sentence* sentence = track->sentences[i];

        char number[9];
        sprintf(number, "%d", i + 1);

        char* timestamp_start = generate_timestamp(sentence->time_start);
        ebml_int time_end = sentence->time_end;
        if (i + 1 < track->sentence_count)
            time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
        char* timestamp_end = generate_timestamp(time_end);

        write(1, number, strlen(number));
        write(1, "\n", 1);
        write(1, timestamp_start, strlen(timestamp_start));
        write(1, " --> ", 5);
        write(1, timestamp_end, strlen(timestamp_start));
        write(1, "\n", 1);
        write(1, sentence->text, sentence->text_size);
        write(1, "\n\n", 2);
    }

    write(1, "\n\n\n\n\n\n\n\n\n\n", 10);
    write(1, "============================================================\n", 31);
    write(1, "\n\n\n\n\n\n\n\n\n\n", 10);
}

void save_all_sub_tracks() {
    int index = 0;
    while (sub_tracks[index] != NULL) {
        save_sub_track(sub_tracks[index]);
        index++;
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

    save_all_sub_tracks();
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