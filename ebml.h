#ifndef EBML_PARSER_H
#define EBML_PARSER_H

#include <stdio.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

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

/* Segment cluster ids */
#define EBML_SEGMENT_CLUSTER_TIMECODE 0xE7
#define EBML_SEGMENT_CLUSTER_SILENT_TRACKS 0x5854
#define EBML_SEGMENT_CLUSTER_POSITION 0xA7
#define EBML_SEGMENT_CLUSTER_PREV_SIZE 0xAB
#define EBML_SEGMENT_CLUSTER_SIMPLE_BLOCK 0xA3
#define EBML_SEGMENT_CLUSTER_ENCRYPTED_BLOCK 0xAF

/* Segment cluster block group ids */
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP 0xA0
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK 0xA1
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_VIRTUAL 0xA2
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_ADDITIONS 0x75A1
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_DURATION 0x9B
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_PRIORITY 0xFA
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK 0xFB
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_CODEC_STATE 0xA4
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_DISCARD_PADDING 0x75A2
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_SLICES 0x8E
#define EBML_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_FRAME 0xC8

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

/* Misc ids */
#define EBML_VOID 0xEC
#define EBML_CRC32 0xBF

/* DEFENCE FROM THE FOOL - deprecated IDs */
#define EBML_SEGMENT_TRACK_TRACK_TIMECODE_SCALE 0x23314F
#define EBML_SEGMENT_TRACK_TRACK_OFFSET 0x537F

/* DivX trick track extenstions (in track entry) */
#define EBML_SEGMENT_TRACK_TRICK_TRACK_UID 0xC0
#define EBML_SEGMENT_TRACK_TRICK_TRACK_SEGMENT_UID 0xC1
#define EBML_SEGMENT_TRACK_TRICK_TRACK_FLAG 0xC6
#define EBML_SEGMENT_TRACK_TRICK_MASTER_TRACK_UID 0xC7
#define EBML_SEGMENT_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID 0xC4

/* Other defines */
#define EBML_MAX_ID_LENGTH 4
#define EBML_TRACK_TYPE_CODE_SUBTITLE 0x11
#define EBML_MAX_TRACKS 128

/* Messages */
#define EBML_INFO "Matroska parser info: "
#define EBML_WARNING "Matroska parser warning: "
#define EBML_ERROR "Matroska parser error: "

/* Boilerplate code */
#define EBML_SWITCH_BREAK(a,b) (a)=0;(b)=0;break

/* Typedefs */
typedef unsigned long ebml_int;
typedef unsigned char ebml_byte;

/* Structures */
struct ebml_sub_sentence {
    ebml_byte* text;
    ebml_int text_size;
    ebml_int time_start;
    ebml_int time_end;
};

struct ebml_sub_track {
    ebml_int track_number;
    ebml_byte* lang;
    ebml_int lang_index;

    int sentence_count;
    struct ebml_sub_sentence** sentences;
};

/* Functions */
void skip_bytes(FILE* file, ebml_int n);
void set_bytes(FILE* file, ebml_int n);
ebml_int get_current_byte(FILE* file);
ebml_byte* read_bytes(FILE* file, ebml_int n);
ebml_byte read_byte(FILE* file);

ebml_int read_vint_length(FILE* file);
ebml_byte* read_vint_block(FILE* file);
ebml_int read_vint_block_int(FILE* file);
ebml_byte* read_vint_block_string(FILE* file);
void read_vint_block_skip(FILE* file);

char* get_track_entry_type_description(ebml_int type);

void parse_ebml(FILE* file);
void parse_segment_info(FILE* file);
struct ebml_sub_sentence* parse_segment_cluster_block_group_block(FILE* file, ebml_int cluster_timecode);
void parse_segment_cluster_block_group(FILE* file, ebml_int cluster_timecode);
void parse_segment_cluster(FILE* file);
void parse_segment_track_entry(FILE* file);
void parse_segment_tracks(FILE* file);
void parse_segment(FILE* file);
void parse(FILE* file);

#endif //EBML_PARSER_H
