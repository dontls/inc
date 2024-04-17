/*
 * @Author: wanguandong
 * @Date: 2018-12-29 14:59:17
 * @LastEditors: wangguandong
 * @LastEditTime: 2019-01-04 11:15:36
 * @Description: original file https://github.com/progschj/threadsafe_queue.h
 */
#pragma once

#include <condition_variable>
#include <initializer_list>
#include <mutex>
#include <queue>

/**
 * 线程安全队列
 * T为队列元素类型
 * 因为有std::mutex和std::condition_variable类成员，所以此类型不支持复制构造函数也不支持赋值操作(=)
 */

namespace libthread {
template <typename T> class Queue {
private:
  mutable std::mutex mtx;
  mutable std::condition_variable cv;
  using queue_type = std::queue<T>;
  queue_type data;
  Queue() = default;
  Queue(const Queue &) = delete;
  Queue &operator=(const Queue &) = delete;

public:
  using value_type = typename queue_type::value_type;
  using container_type = typename queue_type::container_type;

  /**
   * @description: 使用迭代器为参数的构造函数，适用所有容器对象
   * @param {type}
   * @return:
   */
  template <typename _inputiterator>
  Queue(_inputiterator first, _inputiterator last) {
    for (auto itr = first; itr != last; ++itr) {
      data.push(*itr);
    }
  }
  explicit Queue(const container_type &c) : data(c) {}

  /**
   * @description: 使用初始化列表为参数的构造函数
   * @param {type}
   * @return:
   */
  Queue(std::initializer_list<value_type> list)
      : Queue(list.begin(), list.end()) {}

  /**
   * @description: 将元素加入队列
   * @param {type}
   * @return:
   */
  void Push(const value_type &new_value) {
    std::lock_guard<std::mutex> lk(mtx);
    data.push(std::move(new_value));
    cv.notify_one();
  }

  /**
   * @description: 从队列中弹出一个元素，如果队列为空就阻塞
   * @param {type}
   * @return:
   */
  value_type WaitPop() {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [this] { return !this->data.empty(); });
    auto value = std::move(data.front());
    data.pop();
    return value;
  }

  /**
   * @description: 从队列中弹出一个元素，如果队列为空返回false
   * @param {type}
   * @return:
   */
  bool TryPop(value_type &value) {
    std::lock_guard<std::mutex> lk(mtx);
    if (data.empty())
      return false;
    value = std::move(data.front());
    data.pop();
    return true;
  }

  /**
   * @description: 返回队列是否为空
   * @param {type}
   * @return:
   */
  auto Empty() const -> decltype(data.empty()) {
    std::lock_guard<std::mutex> lk(mtx);
    return data.empty();
  }

  /**
   * @description: 返回队列中的元素个数
   * @param {type}
   * @return:
   */
  auto Size() const -> decltype(data.size()) {
    std::lock_guard<std::mutex> lk(mtx);
    return data.size();
  }
};
} // namespace libthread
