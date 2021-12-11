#ifndef SPONGE_LIBSPONGE_TUN_HH
#define SPONGE_LIBSPONGE_TUN_HH

#include "file_descriptor.hh"
#include "util.hh"

#include <cstring>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <string>


//! A FileDescriptor to a [Linux TUN](https://www.kernel.org/doc/Documentation/networking/tuntap.txt) device
class TunFD : public FileDescriptor  {
  public:
    //! Open an existing persistent [TUN device](https://www.kernel.org/doc/Documentation/networking/tuntap.txt).
    explicit TunFD(const std::string &devname)
    : FileDescriptor(SystemCall("open", open(CLONEDEV, O_RDWR))) {
    struct ifreq tun_req {};

    tun_req.ifr_flags = IFF_TUN | IFF_NO_PI;  

    // copy devname to ifr_name, making sure to null terminate

    strncpy(static_cast<char *>(tun_req.ifr_name), devname.data(), IFNAMSIZ - 1);
    tun_req.ifr_name[IFNAMSIZ - 1] = '\0';

    SystemCall("ioctl", ioctl(fd_num(), TUNSETIFF, static_cast<void *>(&tun_req)));
  }

  private:
    static constexpr const char *CLONEDEV = "/dev/net/tun";
};


#endif  // SPONGE_LIBSPONGE_TUN_HH
