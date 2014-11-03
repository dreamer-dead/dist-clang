#pragma once

#include <base/attributes.h>
#include <base/testable.h>
#include <net/connection_forward.h>

namespace dist_clang {
namespace net {

class NetworkServiceImpl;

class NetworkService
    : public base::Testable<NetworkService, NetworkServiceImpl> {
 public:
  using ListenCallback = Fn<void(ConnectionPtr)>;

  virtual ~NetworkService() {}

  virtual bool Run() THREAD_UNSAFE = 0;

  virtual bool Listen(const String& path, ListenCallback callback,
                      String* error = nullptr) THREAD_UNSAFE = 0;
  virtual bool Listen(const String& host, ui16 port, ListenCallback callback,
                      String* error = nullptr) THREAD_UNSAFE = 0;

  virtual ConnectionPtr Connect(EndPointPtr end_point,
                                String* error = nullptr) THREAD_SAFE = 0;
};

}  // namespace net
}  // namespace dist_clang
