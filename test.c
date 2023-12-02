#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/vm_sockets.h>

#ifndef NDEBUG
#define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("usage: %s [PORT] [RETRY_S]\n", argv[0]);
    exit(EXIT_SUCCESS);
  }
  int port = atoi(argv[1]);
  int retry_s = atoi(argv[2]);

  /**
   * Get CID through ioctl() on /dev/vsock
   *
   * getsockname() will not return valid CID without explicit binding CID #!
   */
  int vsock_fd = open("/dev/vsock", O_RDONLY);
  if (vsock_fd < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  unsigned int self_cid;
  if (ioctl(vsock_fd, IOCTL_VM_SOCKETS_GET_LOCAL_CID, &self_cid)) {
    perror("ioctl");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_vm server_addr = {0};
  server_addr.svm_family = AF_VSOCK;
  server_addr.svm_cid = VMADDR_CID_HOST;
  server_addr.svm_port = port;
  while (1) {
    int client_sock;
    if ((client_sock = socket(AF_VSOCK, SOCK_STREAM, 0)) < 0) {
      perror("socket");
      exit(EXIT_FAILURE);
    }

    if (connect(client_sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr))) {
      perror("connect: Retrying...");
      sleep(retry_s);
    } else {
      struct sockaddr_vm client_addr;
      socklen_t client_addr_len = sizeof(client_addr);
      if (getsockname(client_sock, (struct sockaddr *)&client_addr,
                      &client_addr_len)) {
        perror("getsockname");
        exit(EXIT_FAILURE);
      }

      uint64_t host_hash;
      recv(client_sock, &host_hash, sizeof(host_hash), MSG_WAITALL);

      DEBUG_PRINT("[src_cid = %u, src_port = %u] connect: Connected! "
                  "(host_hash <- 0x%lx)\n",
                  self_cid, client_addr.svm_port, host_hash);

      char dummy;
      ssize_t ret = recv(client_sock, &dummy, sizeof(dummy), 0);
      assert(ret <= 0);
      DEBUG_PRINT("recv returned %ld: %s\n", ret, strerror(errno));
    }

    if (close(client_sock)) {
      perror("close");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}
