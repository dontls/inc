#pragma once

// c++封装的单例模式

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <iostream>
#include <memory>
#include <mutex>

template <typename T> class CommSingleton {
  public:
    template <typename... Args>
    static std::shared_ptr<T> NewInstance(Args &&... args);
    static std::shared_ptr<T> GetInstance();
    static void               ReleaseInstance();

  private:
    explicit CommSingleton()         = default;
    CommSingleton(const CommSingleton &) = delete;
    CommSingleton &operator=(const CommSingleton &) = delete;
    ~CommSingleton()                            = default;
    static std::shared_ptr<T> m_sInstance;
};

template <typename T>
std::shared_ptr<T> CommSingleton<T>::m_sInstance = nullptr;

// 构造单例对象，可以传入构造参数
template <typename T>
template <typename... Args>
std::shared_ptr<T> CommSingleton<T>::NewInstance(Args &&... args)
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        m_sInstance = std::make_shared<T>(std::forward<Args>(args)...);
    });
    return m_sInstance;
}

// 获取单例对象
template <typename T> std::shared_ptr<T> CommSingleton<T>::GetInstance()
{
    if (m_sInstance == nullptr) {
        return nullptr;
    }
    return m_sInstance;
}

// 析构单例对象
template <typename T> void CommSingleton<T>::ReleaseInstance()
{
    if (m_sInstance) {
        m_sInstance.reset();
        m_sInstance = nullptr;
    }
}

#endif