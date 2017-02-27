#include "inet.h"

#include <limits.h>
#include <errno.h>

#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <string>
#include <iostream>

#include "test.pb.h"

using namespace std;

#define BLOCK_SIZE 1024
#define BLOCK_SIZE_WORK 2048

char rootdir[PATH_MAX] = "rootdir";

static void fullpath(char fpath[PATH_MAX], const char *path);
static void fullpath(char fpath[PATH_MAX], const char *path)
{
  strcpy(fpath, rootdir);
  strncat(fpath, path, PATH_MAX);
}

int main (int argc, char *argv[])
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int sockfd, newsockfd, aa;
  struct sockaddr_in  cli_addr, serv_addr;
  char buff[BLOCK_SIZE];
  string str_recv, str_send;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
     printf("Server : can’t open stream socket\n");
    ::exit (0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(SERV_TCP_PORT);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    printf("Server : can’t bind local address \n");
    ::exit(0);
  }

  listen(sockfd, 100);

  while (1) { 
    char buff_work[BLOCK_SIZE] = {0,};
    char buff_ack[BLOCK_SIZE] = {0,};

    aa = sizeof(cli_addr);
    newsockfd = :: accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&aa);
    //printf("sockfd : %d\n", newsockfd);

    if (newsockfd < 0) {
      printf("Server : accept error  \n");
      ::exit(0);
    }

    ::memset(buff, 0, BLOCK_SIZE);
    if (::read(newsockfd, buff, BLOCK_SIZE) <= 0) {
      printf("Server : readn error\n");
      ::exit(1);
    }

    //printf("%s\n", buff);
    /*if (strcmp(buff, "break") == 0) {
      break;
    }*/

    ::memset (buff_work, 0, BLOCK_SIZE);
    ::memcpy (buff_work, buff, strlen(buff));

    /////// Case of getattr
    if (strcmp(buff_work, "getattr") == 0) {

      FormatTransfer::getattr obj_getattr_recv, obj_getattr_send;
      char fpath[PATH_MAX];
      struct stat st;
      int ret;

      if (::write(newsockfd, "ACK-getattr", 11) <= 0) {
        printf("Getattr : [ACK-getattr] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_getattr_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_getattr_recv.path().c_str());
      }
      else {
        printf("Getattr : [Read-path] error\n");
        ::exit(1);
      }
      
      //ret = lstat(fpath, &st);
      if ( (ret = lstat(fpath, &st)) == 0 ) {
        obj_getattr_send.set_dev(st.st_dev);
        obj_getattr_send.set_inode(st.st_ino);
        obj_getattr_send.set_mode(st.st_mode);
        obj_getattr_send.set_nlink(st.st_nlink);
        obj_getattr_send.set_uid(st.st_uid);
        obj_getattr_send.set_gid(st.st_gid);
        //obj_getattr_send.set_devid(st.st_rdev);

        obj_getattr_send.set_size(st.st_size);
        obj_getattr_send.set_blksize(st.st_blksize);
        obj_getattr_send.set_nblk(st.st_blocks);
        obj_getattr_send.set_atime(st.st_atime);
        obj_getattr_send.set_mtime(st.st_mtime);
        obj_getattr_send.set_ctime(st.st_ctime);

        obj_getattr_send.set_ret(ret);
        //cout << "GETATTR : " << fpath << " " << obj_getattr_send.size();
        //cout << " " << st.st_size << endl;
      }
      else {
        obj_getattr_send.set_ret(-errno);
      }

      obj_getattr_send.SerializeToString(&str_send);
      
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Getattr : [Final Write - lstat] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);
      
    }        
    
    ///////// Case of opendir
    else if (strcmp(buff_work, "opendir") == 0) {

      FormatTransfer::opendir obj_opendir_recv, obj_opendir_send;
      char fpath[PATH_MAX];
      DIR *dp;
      int ret;

      if (::write(newsockfd, "ACK-opendir", 11) <= 0) {
        printf("Getattr : [ACK-opendir] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_opendir_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_opendir_recv.path().c_str());
      }
      else {
        printf("Opendir : [Read-path] error\n");
        ::exit(1);
      }

      /////// Open directory from full path
      dp = opendir(fpath);

      if (dp == NULL) {
        obj_opendir_send.set_ret(false);
      }
      else {
        obj_opendir_send.set_ret(true);
        obj_opendir_send.set_fd((intptr_t)dp);
      }

      obj_opendir_send.SerializeToString(&str_send);
      
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Opendir : [Final Write - DIR*] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    /////// Case of readdir function
    else if (strcmp(buff_work, "readdir") == 0) {

      DIR *dp;
      struct dirent *de;
      struct stat st;
      FormatTransfer::readdir obj_readdir_recv, obj_readdir_send;
      char fpath[PATH_MAX];

      ////// send ACK
      if (::write(newsockfd, "ACK-readdir", 11) <= 0) {
        printf("Readdir : [ACK-readdir] error\n");
        ::exit(1);
      }

      /////// read path
      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_readdir_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_readdir_recv.path().c_str());
      }
      else {
        printf("Readdir : [Read-path] error\n");
        ::exit(1);
      }

      if ( (dp = opendir(fpath)) != NULL ) {
        de = readdir(dp);
        if (de == 0) {
          obj_readdir_send.set_retentry(-errno);
          obj_readdir_send.SerializeToString(&str_send);
          ::write(newsockfd, str_send.c_str(), str_send.length());
        }

        else {
          do {
            string str_send, str_recv;

            ////// Initialization 
            obj_readdir_send.set_filename(de->d_name);
            obj_readdir_send.set_end(false);
            obj_readdir_send.set_retentry(1);
            str_send = "";  str_recv = "";
            
            obj_readdir_send.SerializeToString(&str_send);

            if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
              printf("Readdir : [Write filename] error\n");
            }

            ::read(newsockfd, buff_ack, 3);

            //obj_readdir_send.clear_filename();
            //obj_readdir_send.clear_end();
            //obj_readdir_send.clear_retentry();

          } while ( (de = readdir(dp)) != NULL );

          obj_readdir_send.set_end(true);
          obj_readdir_send.set_retentry(1);
          obj_readdir_send.SerializeToString(&str_send);
          if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
            printf("Readdir : [Final write - end of loop] error\n");
            ::exit(1);
          }

        }

      }
      ::memset (buff_work, 0, BLOCK_SIZE);
    }

    else if (strcmp(buff_work, "releasedir") == 0) {

      FormatTransfer::release obj_releasedir_recv, obj_releasedir_send;
      char fpath[PATH_MAX];
      int ret;

      if (::write(newsockfd, "ACK-releasedir", 14) <= 0) {
        printf("Releasedir : [ACK-releasedir] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_releasedir_recv.ParseFromString(str_recv);

        //fullpath(fpath, obj_release_recv.path().c_str());
      }
      else {
        printf("Releasedir : [Read-path] error\n");
        ::exit(1);
      }

      ret = ::closedir( (DIR*) (uintptr_t) obj_releasedir_recv.fd() );
      obj_releasedir_send.set_ret(ret);
      obj_releasedir_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Releasedir : [Final write - return value of closedir] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "access") == 0) {

      FormatTransfer::access obj_access_recv, obj_access_send;
      char fpath[PATH_MAX];
      int ret;

      if (::write(newsockfd, "ACK-access", 10) <= 0) {
        printf("Access : [ACK-access] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_access_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_access_recv.path().c_str());
      }
      else {
        printf("Access : [Read-path] error\n");
        ::exit(1);
      }

      ret = ::access(fpath, obj_access_recv.mask());
      if (ret == 0)
        obj_access_send.set_ret(true);
      else
        obj_access_send.set_ret(false);

      obj_access_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Access : [Final write - return value of access] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "open") == 0) {

      FormatTransfer::open obj_open_recv, obj_open_send;
      char fpath[PATH_MAX];
      int ret;

      if (::write(newsockfd, "ACK-open", 8) <= 0) {
        printf("Open : [ACK-open] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_open_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_open_recv.path().c_str());
      }
      else {
        printf("Open : [Read-path] error\n");
        ::exit(1);
      }

      ret = ::open(fpath, obj_open_recv.mode());
      
      obj_open_send.set_fd(ret);
      if (ret > 0)
        obj_open_send.set_ret(true);
      else
        obj_open_send.set_ret(false);

      obj_open_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Open : [Final write - return fd of open] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);
    }

    else if (strcmp(buff_work, "mknod1") == 0) {

      FormatTransfer::mknod obj_mknod_recv, obj_mknod_send;
      char fpath[PATH_MAX];
      mode_t mode;
      dev_t dev;
      int ret;

      if (::write(newsockfd, "ACK-mknod1", 10) <= 0) {
        printf("Mknod : [ACK-mknod1] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_mknod_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_mknod_recv.path().c_str());
        mode = obj_mknod_recv.mode();

        //cout << "[mknod] mode : " << mode << endl;
      }
      else {
        printf("Mknod : [Read-path] error\n");
        ::exit(1);
      }

      ////// Case of regular file
      if (strcmp(obj_mknod_recv.command().c_str(), "open") == 0) {
        ret = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (ret >= 0) {
          //cout << "Right end" << endl;
          ret = close(ret);
          //cout << "ret : " << ret << endl;
        }
      }
      else if (strcmp(obj_mknod_recv.command().c_str(), "mkfifo") == 0) {
        ret = mkfifo(fpath, mode);
      }
      else if (strcmp(obj_mknod_recv.command().c_str(), "mknod") == 0) {
        dev = obj_mknod_recv.dev();
        ret = mknod(fpath, mode, dev);
      }

      obj_mknod_send.set_ret(ret);
      obj_mknod_send.SerializeToString(&str_send);

      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Mknod : [Final write - return value] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
    }

    else if (strcmp(buff_work, "release") == 0) {

      FormatTransfer::release obj_release_recv, obj_release_send;
      char fpath[PATH_MAX];
      int ret;

      if (::write(newsockfd, "ACK-release", 11) <= 0) {
        printf("Release : [ACK-release] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_release_recv.ParseFromString(str_recv);

        //fullpath(fpath, obj_release_recv.path().c_str());
      }
      else {
        printf("Release : [Read-path] error\n");
        ::exit(1);
      }

      ret = ::close(obj_release_recv.fd());
      obj_release_send.set_ret(ret);
      obj_release_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Release : [Final write - return value of close] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "read") == 0) {

      FormatTransfer::read_write obj_read_recv, obj_read_send;
      char read_buf[BLOCK_SIZE_WORK] = {0,};
      int fd, ret;
      size_t size;
      off_t offset;

      if (::write(newsockfd, "ACK-read", 8) <= 0) {
        printf("Read : [ACK-read] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_read_recv.ParseFromString(str_recv);

        fd = obj_read_recv.fd();
        size = obj_read_recv.size();
        offset = obj_read_recv.offset();

        /*cout << "[read] fd : " << fd;
        cout << ", size : " << size;
        cout << ", offset : " << offset << endl;*/

      }
      else {
        printf("Read : [Read-path] error\n");
        ::exit(1);
      }

      while (size > 0) {
        ret = pread (fd, read_buf, BLOCK_SIZE, offset);

        /*cout << "[read]In loop - ";
        cout << "size : " << size << ", offset : " << offset;
        cout << ", ret(should be BLOCK_SIZE) : " << ret << endl;*/

        obj_read_send.set_buffer(read_buf);
        //cout << ", And the buffer : " << read_buf << endl;
        obj_read_send.set_ret(ret);
        obj_read_send.SerializeToString(&str_send);
        //cout << "Length : " << str_send.length() << endl;
        if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
          printf("Read : [Final write - return value of read] error\n");
          ::exit(1);
        }

        ::read(newsockfd, buff_ack, 3);

        ::memset (read_buf, 0, BLOCK_SIZE_WORK);

        size -= BLOCK_SIZE;
        offset += BLOCK_SIZE;
      }
      ::memset (buff_work, 0, BLOCK_SIZE);
    }

    else if (strcmp(buff_work, "write") == 0) {

      FormatTransfer::read_write obj_write_recv, obj_write_send;
      char read_buf[BLOCK_SIZE_WORK] = {0,};
      size_t size;
      off_t offset;
      int fd, ret;

      if (::write(newsockfd, "ACK-write", 9) <= 0) {
        printf("Write : [ACK-write] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);

      while (1) {
        if (::read(newsockfd, read_buf, BLOCK_SIZE_WORK) > 0) {
          str_recv = read_buf;
          obj_write_recv.ParseFromString(str_recv);

          fd = obj_write_recv.fd();
          size = obj_write_recv.size();
          offset = obj_write_recv.offset();

          /*cout << "[write] fd : " << fd;
          cout << ", size : " << size;
          cout << ", offset : " << offset << endl;*/
        }
        else {
          printf("Write : [Read-fd] error\n");
          ::exit(1);
        }

        ///// My birthday is 323. It's symbol of end writing
        if (offset == -323) {
          break;
        }

        ret = pwrite (fd, obj_write_recv.buffer().c_str(), size, offset);

        obj_write_send.set_ret(ret);
        obj_write_send.SerializeToString(&str_send);
        if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
          printf("Write: [Final write - return value of write] error\n");
          ::exit(1);
        }
      }
      ::memset (buff_work, 0, BLOCK_SIZE);
    }

    else if (strcmp(buff_work, "unlink") == 0) {

      FormatTransfer::unlink obj_unlink_recv, obj_unlink_send;
      char fpath[PATH_MAX];
      int ret;

      if (::write(newsockfd, "ACK-unlink", 10) <= 0) {
        printf("Unlink : [ACK-unlink] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_unlink_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_unlink_recv.path().c_str());
      }
      else {
        printf("Unlink : [Read-path] error\n");
        ::exit(1);
      }

      ret = ::unlink(fpath);
      
      obj_unlink_send.set_ret(ret);
      if (ret == 0)
        obj_unlink_send.set_ret(true);
      else
        obj_unlink_send.set_ret(false);

      obj_unlink_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Unlink : [Final write - return of unlink] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "symlink") == 0) {

      FormatTransfer::symlink obj_symlink_recv, obj_symlink_send;
      char fpath[PATH_MAX], linkpath[PATH_MAX] = {0,};
      int ret;

      if (::write(newsockfd, "ACK-symlink", 11) <= 0) {
        printf("Symlink : [ACK-symlink] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_symlink_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_symlink_recv.path().c_str());
        fullpath(linkpath, obj_symlink_recv.linkpath().c_str());
      }
      else {
        printf("Symlink : [Read-path and linkpath] error\n");
        ::exit(1);
      }

      ret = ::symlink(fpath, linkpath);
      
      if (ret == 0)
        obj_symlink_send.set_ret(true);
      else
        obj_symlink_send.set_ret(false);

      obj_symlink_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Symlink : [Final write - return of symlink] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "fgetattr") == 0) {

      FormatTransfer::fgetattr obj_fgetattr_recv, obj_fgetattr_send;
      int fd;
      struct stat st;
      int ret;

      if (::write(newsockfd, "ACK-fgetattr", 12) <= 0) {
        printf("Fgetattr : [ACK-fgetattr] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_fgetattr_recv.ParseFromString(str_recv);

        fd = obj_fgetattr_recv.fd();
      }
      else {
        printf("Fgetattr : [Read-fd] error\n");
        ::exit(1);
      }
      
      if ( (ret = fstat(fd, &st)) == 0 ) {
        obj_fgetattr_send.set_dev(st.st_dev);
        obj_fgetattr_send.set_inode(st.st_ino);
        obj_fgetattr_send.set_mode(st.st_mode);
        obj_fgetattr_send.set_nlink(st.st_nlink);
        obj_fgetattr_send.set_uid(st.st_uid);
        obj_fgetattr_send.set_gid(st.st_gid);
        //obj_getattr_send.set_devid(st.st_rdev);
        obj_fgetattr_send.set_size(st.st_size);
        obj_fgetattr_send.set_blksize(st.st_blksize);
        obj_fgetattr_send.set_nblk(st.st_blocks);
        obj_fgetattr_send.set_atime(st.st_atime);
        obj_fgetattr_send.set_mtime(st.st_mtime);
        obj_fgetattr_send.set_ctime(st.st_ctime);
      }
      obj_fgetattr_send.set_ret(ret);

      obj_fgetattr_send.SerializeToString(&str_send);
      
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Fgetattr : [Final Write - fstat] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);
      
    }

    else if (strcmp(buff_work, "truncate") == 0) {

      FormatTransfer::truncate obj_truncate_recv, obj_truncate_send;
      char fpath[PATH_MAX];
      off_t newsize;
      int ret;

      if (::write(newsockfd, "ACK-truncate", 12) <= 0) {
        printf("Truncate : [ACK-truncate] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_truncate_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_truncate_recv.path().c_str());
        newsize = obj_truncate_recv.size();
      }
      else {
        printf("Truncate : [Read-path and newsize] error\n");
        ::exit(1);
      }

      ret = ::truncate(fpath, newsize);

      if (ret == 0)
        obj_truncate_send.set_ret(true);
      else
        obj_truncate_send.set_ret(false);

      obj_truncate_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Truncate : [Final write - return value of truncate] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "fsync") == 0) {

      FormatTransfer::fsync obj_fsync_recv, obj_fsync_send;
      int fd, ret;

      if (::write(newsockfd, "ACK-fsync", 9) <= 0) {
        printf("Fsync : [ACK-fsync] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_fsync_recv.ParseFromString(str_recv);

        fd = obj_fsync_recv.fd();
      }
      else {
        printf("Fsync : [Read-fd] error\n");
        ::exit(1);
      }

      ret = ::fsync(fd);
      
      if (ret == 0)
        obj_fsync_send.set_ret(true);
      else
        obj_fsync_send.set_ret(false);

      obj_fsync_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Fsync : [Final write - return of fsync] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "chmod") == 0) {

      FormatTransfer::chmod obj_chmod_recv, obj_chmod_send;
      char fpath[PATH_MAX];
      mode_t mode;
      int ret;

      if (::write(newsockfd, "ACK-chmod", 9) <= 0) {
        printf("Chmod : [ACK-chmod] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_chmod_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_chmod_recv.path().c_str());
        mode = obj_chmod_recv.mode();
      }
      else {
        printf("Chmod : [Read-path and mode] error\n");
        ::exit(1);
      }

      ret = ::chmod(fpath, mode);
      
      if (ret == 0)
        obj_chmod_send.set_ret(true);
      else
        obj_chmod_send.set_ret(false);

      obj_chmod_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Chmod : [Final write - return of chmod] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }

    else if (strcmp(buff_work, "readlink") == 0) {

      FormatTransfer::readlink obj_readlink_recv, obj_readlink_send;
      char fpath[PATH_MAX], linkpath[PATH_MAX];
      size_t size;
      int ret;

      if (::write(newsockfd, "ACK-readlink", 12) <= 0) {
        printf("Readlink : [ACK-readlink] error\n");
        ::exit(1);
      }

      ::memset (buff_work, 0, BLOCK_SIZE);
      if (::read(newsockfd, buff_work, BLOCK_SIZE) > 0) {
        str_recv = buff_work;
        obj_readlink_recv.ParseFromString(str_recv);

        fullpath(fpath, obj_readlink_recv.path().c_str());
        fullpath(linkpath, obj_readlink_recv.linkpath().c_str());
        size = obj_readlink_recv.size();
      }
      else {
        printf("Readlink : [Read-path and link and size] error\n");
        ::exit(1);
      }

      ret = ::readlink(fpath, linkpath, size);
      
      obj_readlink_send.set_ret(ret);

      obj_readlink_send.SerializeToString(&str_send);
      if (::write(newsockfd, str_send.c_str(), str_send.length()) < str_send.length()) {
        printf("Readlink : [Final write - return of readlink] error\n");
        ::exit(1);
      }
      ::memset (buff_work, 0, BLOCK_SIZE);

    }
    
    ::close(newsockfd);
  }
  ::close(sockfd);

  google::protobuf::ShutdownProtobufLibrary();
}
