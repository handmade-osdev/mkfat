
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>

void typedef (dir_recurse_f)(
    void *user_data,
    u1* rname,
    u1* fname,
    bool is_dir_flag
);

fn u1 *fname_from_rname(u1 *rname)
{
    u1 *last = rname;
    while (*rname) {
        u1 chara = *rname++;
        if (chara == '/' || chara == '\\') {
            last = rname;
        }
    }
    return last;
}

fn void dir_recurse(
    void *ptr,
    u1 *rname,
    u1 *fname,
    dir_recurse_f *user_recurse)
{

    int rname_length = strlen(rname);
    char *wildcard;

    // Has separator ?
    if (rname[rname_length-1] == '\\' || rname[rname_length-1] == '/') {
        wildcard = "*";
    }
    else {
        wildcard = "\\*";
    }

    u1 wildcard_path[1024] = { 0 };
    snprintf(wildcard_path, 1024, "%s%s", rname, wildcard);

    WIN32_FIND_DATA data;
    HANDLE file = FindFirstFile(wildcard_path, &data);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening directory %s\n", rname);
        return;
    }

    do {
        bool is_directory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        u1 *name = data.cFileName;

        u1 new_rname[1024] = { 0 };
        u8 rname_len = strlen(rname);
        int end;
        int wrote = snprintf(new_rname, 1024, "%s\\%s%n", rname, name, &end);
        if (end != wrote) {
            fprintf(stderr, "error: file in '%s': filename too long\n", fname);
            continue;
        }

        if (strcmp(data.cFileName, ".") != 0 && strcmp(data.cFileName, "..") != 0) {
            user_recurse(ptr, new_rname, name, is_directory);
        }
    } while(FindNextFile(file, &data) != 0);
}
