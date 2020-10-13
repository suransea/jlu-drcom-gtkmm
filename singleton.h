//
// Created by sea on 12/24/18.
//

#ifndef JLU_DRCOM_SINGLETON_H
#define JLU_DRCOM_SINGLETON_H

#include <memory>
#include <utility>

namespace DrcomClient {

template <typename T> class Singleton {
public:
  Singleton() = delete;

  Singleton(const Singleton &) = delete;

  Singleton &operator=(const Singleton &) = delete;

  template <typename... Args>
  static std::shared_ptr<T> instance(Args &&... args) {
    if (instance_.get() == nullptr)
      instance_.reset(new T(std::forward<Args>(args)...));
    return instance_;
  }

private:
  virtual ~Singleton() = default;

  static std::shared_ptr<T> instance_;
};

template <class T> std::shared_ptr<T> Singleton<T>::instance_ = nullptr;

} // namespace DrcomClient

#endif // JLU_DRCOM_SINGLETON_H
