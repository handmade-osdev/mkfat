#include "mkfat.h"
#include "io.c"
#include "fat.c"

#if defined(P_USE_POSIX)
    #include"platform_posix.c"
#elif defined(P_USE_WINDOWS)
    #include"platform_windows.c"
#else
    #error "bad platform"
#endif

fn void str_copy_nonull(u1 *dest, u1 *src)
{
    while(*src) {
        *dest++=*src++;
    }
}

fn u8 alignf(u8 p, u8 a)
{
    return (p+a-1)&~(a-1);
}

fn void fat_recurse_dir(dir_info *dir, u1 *rname, u1 *fname, bool file_is_dir)
{
    printf("Processing: %s\n", rname);

    u8 node_so;
    u8 node_co;
    u1 *root_sec;
    u1 *dir_clus;

    fat_node *node;
    if(dir->root) {
        if(dir->size == vol.root_ss*512) {
            fprintf(stderr, "Root directory overflow\n");
            exit(1);
        }

        u8 root_so = vol.resv_ss + 2*vol.fat_ss;
        node_so = root_so + (dir->size/512);
        root_sec = read_sec(node_so);
        u8 node_bo = dir->size % 512;

        node = (fat_node *)(root_sec + node_bo);

        dir->size += sizeof(fat_node);
    }
    else {        
        node_co = dir->last_co;
        u8 node_bo = dir->size % vol.clus_bs;
        if(node_bo == 0) {
            node_co = fat_alloc_clus(dir->last_co);
        }
        dir_clus = read_clus(node_co);

        node = (fat_node *)(dir_clus + node_bo);

        dir->last_co = node_co;
        dir->size += sizeof(fat_node);
    }

    fat_format_fname(node->name, fname);

    if(file_is_dir) {
        u8 clus = fat_alloc_clus(vol.first_free);
        node->attr |= ATTR_DIR;
        node->clus_lo = clus&0xffff;
        node->clus_hi = clus>>4;
        node->size = 0;

        fat_node *nodes = read_clus(clus);
        str_copy_nonull(nodes[0].name, ".          ");
        nodes[0].attr = ATTR_DIR;
        nodes[0].size = 0;
        nodes[0].clus_lo = clus&0xffff;
        nodes[0].clus_hi = clus>>16;
        str_copy_nonull(nodes[1].name, "..         ");
        nodes[1].attr = ATTR_DIR;
        nodes[1].size = 0;
        nodes[1].clus_lo = dir->first_co & 0xffff;
        nodes[1].clus_hi = dir->first_co >> 16;
        ditch_clus(nodes, clus);

        dir_info di;
        di.root = false;
        di.size = 2*sizeof(fat_node);
        di.first_co = clus;
        di.last_co = clus;
        di.fat_node = node;

        dir_recurse(&di, rname, fname, (dir_recurse_f *)fat_recurse_dir);
    } else {
        file_info fi;
        fat_write_host_file(&fi, rname);

        node->attr |= ATTR_ARC;
        node->clus_lo = fi.first_co & 0xffff;
        node->clus_hi = fi.first_co >> 16;
        node->size = fi.size;
    }

    if(dir->root) {
        ditch_sec(root_sec, node_so);
    } else {
        ditch_clus(dir_clus, node_co);
    }
}

fn bool is_digit(u1 c)
{
    return '0' <= c && c <= '9';
}

fn u8 parse_size_spec(u1 *spec)
{
    u8 count = 0;
    u8 unit = 0;

    while(is_digit(*spec)) {
        count = 10*count + *spec-'0';
        spec++;
    }

    if(!strcmp(spec, "k")) {
        unit = 1024 / 512;
    }
    else if(!strcmp(spec, "m")) {
        unit = 1024*1024 / 512;
    }
    else if(!strcmp(spec, "g")) {
        unit = 1024ull*1024*1024 / 512;
    }
    else if(*spec == 0){
        unit = 1;
    }
    else {
        fprintf(stderr, "Unknown unit: %s\n", spec);
        exit(1);
    }

    return count*unit;
}

// mkfat type disk root [-b boot]
i4 main(i4 argc, char *argv[])
{
    if(argc < 5) {
        fprintf(stderr, "Format: mkfat <fat-type> <image-size> <image> <root> [-b boot]\n");
        exit(1);
    }

    u1 *fat_name = argv[1];
    u1 *image_size = argv[2];
    u1 *disk_rname = argv[3];
    u1 *root_rname = argv[4];
    u1 *boot_rname = 0;
    if(argc == 5 && !strcmp(argv[4], "-b")) {
        if(argc < 6) {
            fprintf(stderr, "Please provide boot filename after -b\n");
            exit(1);
        }
        boot_rname = argv[5];
    }
    u8 disk_ss = parse_size_spec(image_size);
    
    fat_type type;
    if(!strcmp(fat_name, "12")) {
        type = fat_12;
    } else if(!strcmp(fat_name, "16")) {
        type = fat_16;
    } else if(!strcmp(fat_name, "32")) {
        type = fat_32;
    } else {
        fprintf(stderr, "Unknown FAT type: %s\n", fat_name);
        exit(1);
    }

    fat_decide_params(disk_ss, type);

    vol.image = fopen(disk_rname, "w+b");
    if(!vol.image) {
        fprintf(stderr, "Error opening the image file for writing: %s\n", disk_rname);
        exit(1);
    }

    fseek(vol.image, 512*(vol.disk_ss)-1, SEEK_SET);
    fputc(0, vol.image);
    fflush(vol.image);

    u1 boot[512] = {0};
    if(boot_rname != NULL) {
        FILE *boot_file = fopen(boot_rname, "wb");
        if(!boot_file) {
            fprintf(stderr, "Error opening the boot file for reading: %s\n", boot_rname);
            exit(1);
        }
        fread(boot, 512, 1, boot_file);
        fclose(boot_file);
    }

    vol.bpb = (void *)boot;
    vol.ext_bpb = boot + sizeof(fat_bpb);

    fat_node *root = calloc(vol.root_ss, 512);
    assert(root != NULL);
    vol.root = root;

    u4 *fat_buffer = calloc(4, vol.data_cs);
    assert(fat_buffer != NULL);
    vol.fat = fat_buffer;
    vol.fat[0] = 0x0ffffff8;
    vol.fat[1] = 0xffffffff;
    vol.first_free = 2;

    u1 *start_name = root_rname;

    u8 dir_co;
    dir_info dir = {0};
    dir.size = 0;
    if(vol.type == fat_32) {
        dir.root = false;
        dir.fat_node = NULL;
        dir_co = fat_alloc_clus(vol.first_free);
        dir.last_co = dir_co;
    }
    else {
        dir.root = true;
    }

    dir_recurse(&dir, start_name, fname_from_rname(start_name), (dir_recurse_f *)fat_recurse_dir);

    u8 root_ents = alignf(dir.size, 512)/32;

    str_copy_nonull(vol.bpb->oem, "MKFAT1.0");
    vol.bpb->sec_bs = 512;
    vol.bpb->clus_ss = vol.clus_ss;
    vol.bpb->resv_ss = vol.resv_ss;
    vol.bpb->fats_n = 2;
    vol.bpb->rootents_n = ((vol.type == fat_32)?0:root_ents);
    vol.bpb->disk_ss16 = (vol.type != fat_32 && vol.disk_ss <= 0xffff) ? vol.disk_ss : 0;
    vol.bpb->vol_type = 0xf8;
    vol.bpb->fat_ss16 = ((vol.type != fat_32) && vol.fat_ss <= 0xffff) ? vol.fat_ss : 0;
    vol.bpb->track_ss = 0;
    vol.bpb->heads_n = 0;
    vol.bpb->hidden_ss = 0;
    vol.bpb->disk_ss32 = vol.disk_ss;

    if(vol.type == fat_32) {
        vol.bpb32->fat_ss = vol.fat_ss;
        vol.bpb32->flags = 0;
        vol.bpb32->version = 0;
        vol.bpb32->root_co = dir_co;
        vol.bpb32->fsinfo_so = 1;
        vol.bpb32->backup_so = 0;
        vol.bpb32->drive = 0x80;
        vol.bpb32->nt_flags = 0;
        vol.bpb32->signature = 0x29;
        str_copy_nonull(vol.bpb32->vol_label, "MKFAT   1.0");
        str_copy_nonull(vol.bpb32->fat_label, "FAT32   ");
    } else {
        vol.bpb16->drive = 0x80;
        vol.bpb16->nt_flags = 0;
        vol.bpb16->signature = 0x29;
        vol.bpb16->vol_id = 0;
        str_copy_nonull(vol.bpb16->vol_label, "MKFAT   1.0");
        str_copy_nonull(vol.bpb16->fat_label, "FAT16   ");
    }

    fseek(vol.image, 0, SEEK_SET);
    fwrite(boot, 1, 512, vol.image);
    fflush(vol.image);

    u8 sec_ents_n = fat_ents_per_sec[vol.type];
    for(u8 sec_i = 0; sec_i != vol.fat_ss; ++sec_i) {
        u1 fat_sec[512] = {0};

        for(u8 j = 0; j != sec_ents_n; ++j) {

            u8 clus_i = sec_i*sec_ents_n + j;
            u8 entry;
            if(clus_i < vol.data_cs) {
                entry = vol.fat[clus_i] & fat_entry_mask[vol.type];
            } else {
                entry = 0;
            }

            if(vol.type == fat_32) {
                ((u4 *)fat_sec)[j] = entry;
            } else if(vol.type == fat_16) {
                ((u2 *)fat_sec)[j] = entry;
            } else if(vol.type == fat_12) {
                if(j%2 == 0) {
                    fat_sec[3*j]   = (u1)entry;
                    fat_sec[3*j+1] = entry>>8;
                } else {
                    fat_sec[3*j-3+1] |= (u1)entry << 4;
                    fat_sec[3*j-3+2] = entry>>8;
                }
            }
        }

        fseek(vol.image, 512*(vol.resv_ss+sec_i), SEEK_SET);
        fwrite(fat_sec, 1, 512, vol.image);
        fflush(vol.image);
        fseek(vol.image, 512*(vol.resv_ss+vol.fat_ss+sec_i), SEEK_SET);
        fwrite(fat_sec, 1, 512, vol.image);
        fflush(vol.image);
    }

    fclose(vol.image);
    
    return 0;
}
