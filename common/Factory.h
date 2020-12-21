#pragma once

#ifndef __COMM_FACTORY_H__
#define __COMM_FACTORY_H__

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// T 需要传入基类
template <typename T> class CommFactory {
  public:
    // N 子类，这里会把对应子类添加到Map中
    template <typename N> struct register_t {
        template <typename... Args>
        register_t(const std::string &key, Args... args)
        {
            CommFactory::Instance()->_map.emplace(
                key, [=] { return new N(args...); });
        }
    };

    // 普通指针
    static auto NormalPtr(const std::string &key) -> T *
    {
        return CommFactory::Instance()->Find(key);
    }

    // unique_ptr指针
    static std::unique_ptr<T> UniquePtr(const std::string &key)
    {
        return std::unique_ptr<T>(NormalPtr(key));
    }

    // shared_ptr指针
    static std::shared_ptr<T> SharedPtr(const std::string &key)
    {
        return std::shared_ptr<T>(NormalPtr(key));
    }

  private:
    CommFactory(){};
    CommFactory(const CommFactory &) = delete;
    CommFactory(CommFactory &&)      = delete;
    CommFactory &operator=(const CommFactory &) = delete;

    // 单例模式
    static auto Instance() -> CommFactory<T> *
    {
        static CommFactory<T> ins;
        return &ins;
    }

    auto Find(const std::string &key) -> T *
    {
        if (_map.find(key) == _map.end())
            throw std::invalid_argument("key is not exist!");

        // 这里会动态转化指针，如果不是继承关系，返回nullptr
        return dynamic_cast<T *>(_map[key]());
    }
    std::map<std::string, std::function<T *()>> _map;
};
#endif

// class B {
// public:
//     virtual void display() = 0;
// };
// 注册基类
// #define AF_OBJECT CommFactory<B>

// 注册子类，这里没有对继承关系进行判断，需要保证继承关系
// #define REGISTER_AF_OBJECT_CLASS(CLASS, ...) AF_OBJECT::register_t<CLASS>
// __##CLASS(#CLASS, ##__VA_ARGS__)
