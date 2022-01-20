
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
    // get the short filename from the relative filename.
}

fn void dir_recurse(
    void *ptr,
    u1 *rname,
    u1 *fname,
    dir_recurse_f *user_recurse)
{
    // for each file in directory `fname` call `user_recurse`
    // don't call `user_recurse` with dot and dotdot files.
}
