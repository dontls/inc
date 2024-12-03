#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>

namespace libcomm {
// T 需要传入基类
template <typename T> class Factory {
public:
  // N 子类，这里会把对应子类添加到Map中
  template <typename N> struct register_t {
    template <typename... Args>
    register_t(const std::string &key, Args... args) {
      Factory::Instance()->map_.emplace(key, [=] { return new N(args...); });
    }
  };

  // 普通指针
  static auto NormalPtr(const std::string &key) -> T * {
    return Factory::Instance()->Find(key);
  }

  // unique_ptr指针
  static std::unique_ptr<T> UniquePtr(const std::string &key) {
    return std::unique_ptr<T>(NormalPtr(key));
  }

  // shared_ptr指针
  static std::shared_ptr<T> SharedPtr(const std::string &key) {
    return std::shared_ptr<T>(NormalPtr(key));
  }

private:
  Factory() {};
  Factory(const Factory &) = delete;
  Factory(Factory &&) = delete;
  Factory &operator=(const Factory &) = delete;

  // 单例模式
  static auto Instance() -> Factory<T> * {
    static Factory<T> ins;
    return &ins;
  }

  auto Find(const std::string &key) -> T * {
    if (map_.find(key) == map_.end())
      throw std::invalid_argument("key is not exist!");

    // 这里会动态转化指针，如果不是继承关系，返回nullptr
    return dynamic_cast<T *>(map_[key]());
  }
  std::map<std::string, std::function<T *()>> map_;
};
} // namespace libcomm

// class B {
// public:
//     virtual void display() = 0;
// };
// 注册基类
// #define AF_OBJECT Factory<B>

// 注册子类，这里没有对继承关系进行判断，需要保证继承关系
// #define REGISTER_AF_OBJECT_CLASS(CLASS, ...) AF_OBJECT::register_t<CLASS>
// __##CLASS(#CLASS, ##__VA_ARGS__)
