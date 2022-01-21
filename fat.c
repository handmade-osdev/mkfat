
fn u8 ceil_div(u8 divident, u8 divisor)
{
    return (divident+divisor-1)/divisor;
}

fn u8 fat_subdivide_disk(fat_type type, u8 split_ss, u8 clus_ss, u8 *fat_ss_o, u8 *data_ss_o)
{
    u8 unit = clus_ss*fat_ents_per_sec[type] + 2;
    u8 fat_ss = (split_ss + unit - 1) / unit;
    u8 data_ss = split_ss - 2*fat_ss;
    u8 data_cs = data_ss / clus_ss;
    u8 addr = fat_ss*fat_ents_per_sec[type]-2;
    if(addr < data_cs) {
        u8 secs = clus_ss;
        if(clus_ss % 2 == 0) {
            secs /= 2;
        }
        fat_ss += secs;
        data_ss -= 2*secs;
    }
    *fat_ss_o = fat_ss;
    *data_ss_o = data_ss;
}

fn void fat_decide_params(u8 disk_ss, fat_type type)
{
    u8 resv_ss = 1;
    u8 root_ss = 1;
    if(type == fat_32) root_ss = 0;

    u8 split_ss = disk_ss - resv_ss - root_ss;
    u8 clus_ss = 1;
    u8 fat_ss;
    u8 data_ss;
    u8 data_cs;
    for(;;) {
        fat_subdivide_disk(type, split_ss, clus_ss, &fat_ss, &data_ss);
        data_cs = data_ss / clus_ss;

        if(data_cs < fat_limit_lo[type]) {
                fprintf(stderr, "Disk size %"PRIu64" is too low for %s\n",
                            disk_ss, fat_name[type]);
            exit(1);
        }
        else if(data_cs >= fat_limit_hi[type]) {
            if(clus_ss == 0x80) {
                fprintf(stderr, "Disk size %"PRIu64" is too high for %s\n",
                                disk_ss, fat_name[type]);
                exit(1);
            }
            clus_ss *= 2;
            continue;
        }
        break;
    }

    u8 total_ss = resv_ss + 2*fat_ss + root_ss + data_ss;
    assert(total_ss == disk_ss);

    vol.type = type;
    vol.disk_ss = disk_ss;
    vol.clus_ss = clus_ss;
    vol.clus_bs = clus_ss * 512;
    vol.root_ss = root_ss;
    vol.resv_ss = resv_ss;
    vol.fat_ss = fat_ss;
    vol.data_ss = data_ss;
    vol.data_cs = data_cs;
}

fn u8 fat_find_next_free(u8 clus)
{
    do {
        clus++;
    } while(vol.fat[clus] != 0);
    return clus;
}

fn void fat_mark_clus_last(u8 clus)
{
    vol.fat[clus] = 0x0fffffff;
}

// Note: when `prev` is `first_free`, this function
// allocates a chain of 1 cluster.
fn u8 fat_alloc_clus(u8 prev)
{
    u8 new = vol.first_free;
    vol.first_free = fat_find_next_free(vol.first_free);
    vol.fat[prev] = new;
    vol.fat[new] = 0x0fffffff;
    return new;
}

fn u1 to_upper(u1 c)
{
    if('a' <= c && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

fn bool fat_format_fname(u1 buffer[11], u1 *fname)
{
    u1 *f = fname;

    u1 *name = fname;
    i4 name_n = 0;
    while(!(*f == '.' || *f == 0)) {
        name_n++;
        f++;
    }

    u1 *ext = fname+name_n+1;
    i4 ext_n = 0;
    if(*f == '.') {
        ++f;
        while(*f != 0) {
            f++;
            ext_n++;
        }
    }
    if(name_n > 8 || ext_n > 3) {
        return false;
    }
      
    i4 i = 0;
    for(;i<name_n; ++i) buffer[i]=fname[i];
    for(;i<8;      ++i) buffer[i]=' ';
    i4 j = 0;
    for(;j<ext_n;++j)   buffer[8+j]=ext[j];
    for(;j<3;    ++j)   buffer[8+j]=' ';
    
    return true;
}

fn void fat_write_host_file(file_info *fi, u1 *rname)
{
    FILE *file = fopen(rname, "rb");
    if(!file) {
        fprintf(stderr, "Unable to open file: %s\n", rname);
        exit(1);
    }

    int status = fseek(file, 0, SEEK_END);
    if(status != 0) {
        fprintf(stderr, "Bad file: %s\n", rname);
        exit(1);
    }
    u8 size_bs = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8 size_cs = (size_bs + vol.clus_bs - 1) / vol.clus_bs;
    u8 clus_idx = 0;
    u8 clus_first = vol.first_free;
    u8 clus_last = vol.first_free;
    if(size_cs != 0) do {
        u8 clus_num = fat_alloc_clus(clus_last);
        void *clus = read_clus(clus_num);
        memset(clus, 0, vol.clus_bs);
        fread(clus, 1, vol.clus_bs, file);
        ditch_clus(clus, clus_num);
        clus_last = clus_num;
    } while(++clus_idx != size_cs);
    fat_mark_clus_last(clus_last);

    fi->first_co = clus_first;
    fi->size = size_bs;

    fclose(file);
}

