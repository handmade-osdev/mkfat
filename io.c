
fn u8 clus_to_sec(u8 clus)
{
    u8 data_so = vol.resv_ss + 2*vol.fat_ss + vol.root_ss;
    u8 clus_so = data_so + (clus-2)*vol.clus_ss;
    return clus_so;
}

fn void *read_sec(u8 num)
{
    void *buffer = malloc(512);
    assert(buffer);
    fseek(vol.image, 512*num, SEEK_SET);
    fread(buffer, 1, 512, vol.image);
    return buffer;
}

fn void ditch_sec(void *buffer, u8 num)
{
    fseek(vol.image, 512*num, SEEK_SET);
    fwrite(buffer, 1, 512, vol.image);
    fflush(vol.image);
    free(buffer);
}

fn void *read_clus(u8 num)
{
    void *buffer = malloc(vol.clus_bs);
    assert(buffer);
    u8 clus_so = clus_to_sec(num);
    fseek(vol.image, 512*clus_so, SEEK_SET);
    fread(buffer, 1, vol.clus_bs, vol.image);
    return buffer;
}

fn void ditch_clus(void *buffer, u8 num)
{
    u8 clus_so = clus_to_sec(num);
    fseek(vol.image, 512*clus_so, SEEK_SET);
    fwrite(buffer, 1, vol.clus_bs, vol.image);
    fflush(vol.image);
    free(buffer);
}
