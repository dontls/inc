#pragma once

// c++封装的单例模式

// #include <memory>
// #include <mutex>

// template <typename T> class CSingleton {
// public:
//   template <typename... Args>
//   static std::shared_ptr<T> NewInstance(Args &&...args);
//   static std::shared_ptr<T> GetInstance();
//   static void ReleaseInstance();

// private:
//   explicit CSingleton() = default;
//   CSingleton(const CSingleton &) = delete;
//   CSingleton &operator=(const CSingleton &) = delete;
//   ~CSingleton() = default;
//   static std::shared_ptr<T> m_sInstance;
// };

// template <typename T> std::shared_ptr<T> CSingleton<T>::m_sInstance =
// nullptr;

// // 构造单例对象，可以传入构造参数
// template <typename T>
// template <typename... Args>
// std::shared_ptr<T> CSingleton<T>::NewInstance(Args &&...args) {
//   static std::once_flag onceFlag;
//   std::call_once(onceFlag, [&]() {
//     m_sInstance = std::make_shared<T>(std::forward<Args>(args)...);
//   });
//   return m_sInstance;
// }

// // 获取单例对象
// template <typename T> std::shared_ptr<T> CSingleton<T>::GetInstance() {
//   if (m_sInstance == nullptr) {
//     return nullptr;
//   }
//   return m_sInstance;
// }

// // 析构单例对象
// template <typename T> void CSingleton<T>::ReleaseInstance() {
//   if (m_sInstance) {
//     m_sInstance.reset();
//     m_sInstance = nullptr;
//   }
// }

// c++封装的单例模式
namespace libcomm {

template <typename T> class Singleton {
public:
  static T &Instance() {
    static T ins;
    return ins;
  }

private:
  explicit Singleton() = default;
  Singleton(const Singleton &) = delete;
  Singleton &operator=(const Singleton &) = delete;
  ~Singleton() = default;
};

} // namespace libsingle