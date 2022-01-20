
// Contains hungarian notation to save extra characters on identifiers.
//  _bs = byte size
//  _ss = sector size
//  _cs = cluster size
//  _bo = byte offset
//  _so = sector offset
//  _co = cluster offset
//  _o  = offset
//  _n  = count/number
// And also contains some specific terminology:
//  fname = short filename (name+ext)
//    name = part before suffix
//    ext  = suffix
//  aname = absolute filename
//  rname = relative filename

#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<stdbool.h>
#include<ctype.h>
#include<string.h>
#include<inttypes.h>

int8_t   typedef i1;
int16_t  typedef i2;
int32_t  typedef i4;
int64_t  typedef i8;
uint8_t  typedef u1;
uint16_t typedef u2;
uint32_t typedef u4;
uint64_t typedef u8;

#define fn static

#pragma pack(push, 1)

struct fat_bpb typedef fat_bpb;
struct fat_bpb
{
    u1 jmp[3];
    u1 oem[8];
    u2 sec_bs;
    u1 clus_ss;
    u2 resv_ss;
    u1 fats_n;
    u2 rootents_n;
    u2 disk_ss16;
    u1 vol_type;
    u2 fat_ss16;
    u2 track_ss;
    u2 heads_n;
    u4 hidden_ss;
    u4 disk_ss32;
};

struct fat_bpb_ext16 typedef fat_bpb_ext16;
struct fat_bpb_ext16
{
    u1 drive;
    u1 nt_flags;
    u1 signature;
    u4 vol_id;
    u1 vol_label[11];
    u1 fat_label[8];
};

struct fat_bpb_ext32 typedef fat_bpb_ext32;
struct fat_bpb_ext32
{
    u4 fat_ss;
    u2 flags;
    u2 version;
    u4 root_co;
    u2 fsinfo_so;
    u2 backup_so;
    u1 reserved[12];
    u1 drive;
    u1 nt_flags;
    u1 signature;
    u4 vol_id;
    u1 vol_label[11];
    u1 fat_label[8];
};

#define ATTR_RO  0x01
#define ATTR_HID 0x02
#define ATTR_SYS 0x04
#define ATTR_VOL 0x08
#define ATTR_DIR 0x10
#define ATTR_ARC 0x20

struct fat_node typedef fat_node;
struct fat_node {
    u1 name[11];
    u1 attr;
    u1 resv;
    u1 ctime_secs;
    u2 ctime;
    u2 cdate;
    u2 adate;
    u2 clus_hi;
    u2 mtime;
    u2 mdate;
    u2 clus_lo;
    u4 size;
} packed;

#pragma pack(pop)

enum fat_type typedef fat_type;
enum fat_type {
    fat_12,
    fat_16,
    fat_32,
};

char *fat_name[] = {
    "FAT 12",
    "FAT 16",
    "FAT 32",
};

u8 fat_limit_lo[] = {
    0,
    4086,
    65526,
};

u8 fat_limit_hi[] = {
    4086,
    65536,
    268435454,
};

u8 fat_ents_per_sec[] = {
    512*2/3,
    512/2,
    512/4,
};

u8 fat_entry_mask[] = {
    0xfff,
    0xffff,
    0xffffffff,
};

struct volume typedef volume;
struct volume {
    FILE *image;
    fat_type type;
    u8 disk_ss;
    u8 clus_ss;
    u8 clus_bs;
    u8 resv_ss;
    u8 fat_ss;
    u8 root_ss;
    u8 data_ss;
    u8 data_cs;
    u4 *fat;
    u8 first_free;
    fat_node *root;
    fat_bpb *bpb;
    union {
        fat_bpb_ext16 *bpb16;
        fat_bpb_ext32 *bpb32;
        void *ext_bpb;
    };
};

struct dir_info typedef dir_info;
struct dir_info {
    bool root;
    u8 size;
    u8 first_co;
    u8 last_co;
    fat_node *fat_node;
};

struct file_info typedef file_info;
struct file_info {
    u8 first_co;
    u8 size;
};

static volume vol;
