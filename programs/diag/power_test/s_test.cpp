#include <iostream>

#include <unistd.h>
#include <sys/fcntl.h>

static void create(const char* const file)
{
    unlink(file);
    int fd = open(file, O_WRONLY|O_CREAT, 0666);
    close(fd);
}
int main()
{
    uid_t uid = getuid();
    uid_t gid = getgid();
    uid_t euid = geteuid();
    uid_t egid = getegid();


    std::cout << "uid " << uid << " gid " << gid << " euid " << euid << " egid " << egid << std::endl;
    create("f1.txt");
    seteuid(uid);
    create("f2.txt");
    seteuid(euid);
    create("f3.txt");
    return (0);
}
