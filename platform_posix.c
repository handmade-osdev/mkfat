
#include<sys/times.h>
#include<dirent.h>

void typedef (dir_recurse_f)(void*,u1*,u1*,bool);

fn u1 *fname_from_rname(u1 *rname)
{
    u1 *last = rname;

    u1 *str = rname;
    while(*str) {
        if(*str == '/') {
            ++str;
            last = str;
        }
        ++str;
    }

    return last;
}

fn void dir_recurse(void *ptr, u1 *rname, u1 *fname, dir_recurse_f *user_recurse)
{
    DIR *dir_ptr = opendir(rname);
    if(dir_ptr == NULL) {
        fprintf(stderr, "Error opening directory %s\n", rname);
        return;
    }
    struct dirent *dirent_ptr;
    while ((dirent_ptr = readdir(dir_ptr)) != 0)
    {
        u1 *name = dirent_ptr->d_name;

        u1 new_rname[1024] = {0};

        u8 rname_len = strlen(rname);
        strcpy(new_rname, rname);
        new_rname[rname_len] = '/';

        u8 name_len = strlen(name);
        if(rname_len+1+name_len+1 > sizeof new_rname) {
            fprintf(stderr, "error: file in '%s': filename too long\n", fname);
            continue;
        }

        strcpy(new_rname+rname_len+1, name);

        if(strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
            bool dir = (dirent_ptr->d_type == DT_DIR);
            user_recurse(ptr, new_rname, name, dir);
        }
    }
    closedir(dir_ptr);
}
