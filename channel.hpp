#pragma once
// https://github.com/andreiavrammsd/cpp-channel

#include <cassert>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>

namespace libcomm {

/**
 * @brief An iterator that block the current thread,
 * waiting to fetch elements from the channel.
 *
 * Used to implement channel range-based for loop.
 *
 * @tparam Channel Instance of channel.
 */
template <typename Channel> class BlockingIterator {
public:
  using value_type = typename Channel::value_type;
  using reference = const typename Channel::value_type &;

  explicit BlockingIterator(Channel &chan) : chan_{chan} {}

  /**
   * Advances to next element in the channel.
   */
  BlockingIterator<Channel> operator++() const noexcept { return *this; }

  /**
   * Returns an element from the channel.
   */
  reference operator*() {
    chan_ >> value_;
    return value_;
  }

  /**
   * Makes iteration continue until the channel is closed and empty.
   */
  bool operator!=(BlockingIterator<Channel>) const { return !chan_.closed(); }

private:
  Channel &chan_;
  value_type value_{};
};

// /**
//  * @brief Input iterator specialization
//  */
// template <typename T> struct std::iterator_traits<BlockingIterator<T>> {
//   using value_type = typename BlockingIterator<T>::value_type;
//   using reference = typename BlockingIterator<T>::reference;
//   using iterator_category = std::input_iterator_tag;
// };

#if (__cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

// 由写入方关闭Channel
template <typename T> class Channel {
public:
  using value_type = T;
  using iterator = BlockingIterator<Channel<T>>;
  using size_type = std::int32_t;

  /**
   * Creates an unbuffered channel.
   */
  constexpr Channel() = default;

  /**
   * Creates a buffered channel.
   *
   * @param capacity Number of elements the channel can store before blocking.
   */
  explicit constexpr Channel(size_type capacity);

  /**
   * Pushes an element into the channel.
   *
   * @throws closed_channel if channel is closed.
   */
  template <typename Type>
  friend Channel<typename std::decay<Type>::type> &
  operator<<(Channel<typename std::decay<Type>::type> &, Type &&);

  /**
   * Pops an element from the channel.
   *
   * @tparam Type The type of the elements
   */
  template <typename Type>
  friend Channel<Type> &operator>>(Channel<Type> &, Type &);

  /**
   * Returns the number of elements in the channel.
   */
  NODISCARD inline std::size_t constexpr size() const noexcept {
    return queue_.size();
  }

  /**
   * Returns true if there are no elements in channel.
   */
  NODISCARD inline bool constexpr empty() const noexcept {
    return queue_.empty();
  }

  /**
   * Closes the channel.
   */
  inline void close() noexcept;

  /**
   * Returns true if the channel is closed.
   */
  NODISCARD inline bool closed() const noexcept;

  /**
   * Iterator
   */
  iterator begin() noexcept { return BlockingIterator<Channel<T>>{*this}; }
  iterator end() noexcept { return BlockingIterator<Channel<T>>{*this}; }

  /**
   * Channel cannot be copied or moved.
   */
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;
  Channel(Channel &&) = delete;
  Channel &operator=(Channel &&) = delete;
  virtual ~Channel() = default;

private:
  const size_type cap_{0};
  std::queue<T> queue_;
  size_type size_{0};
  std::mutex mtx_;
  std::condition_variable cnd_;
  std::atomic<bool> is_closed_{false};

  void waitBeforeRead(std::unique_lock<std::mutex> &lock) {
    cnd_.wait(lock, [this]() { return !empty() || closed(); });
  }
  void waitBeforeWrite(std::unique_lock<std::mutex> &lock) {
    cnd_.wait(lock, [this]() { return size_ < cap_; });
  }
  friend class BlockingIterator<Channel>;
};

template <typename T>
constexpr Channel<T>::Channel(const size_type capacity) : cap_{capacity} {
  assert(cap_ >= 0);
}

template <typename T>
Channel<typename std::decay<T>::type> &
operator<<(Channel<typename std::decay<T>::type> &ch, T &&in) {
  if (ch.closed()) {
    throw std::runtime_error{"cannot write on closed channel"};
  }

  {
    std::unique_lock<std::mutex> lock{ch.mtx_};
    ch.waitBeforeWrite(lock);
    ch.queue_.push(std::forward<T>(in));
    ++ch.size_;
  }
  ch.cnd_.notify_one();
  return ch;
}

template <typename T> Channel<T> &operator>>(Channel<T> &ch, T &out) {
  if (ch.closed() && ch.empty()) {
    return ch;
  }
  std::unique_lock<std::mutex> lock{ch.mtx_};
  --ch.size_;
  ch.cnd_.notify_one(); // 写入阻塞，唤醒
  ch.waitBeforeRead(lock);
  if (!ch.empty()) {
    out = std::move(ch.queue_.front());
    ch.queue_.pop();
  }
  return ch;
}

template <typename T> void Channel<T>::close() noexcept {
  {
    std::lock_guard<std::mutex> lock{mtx_};
    is_closed_.store(true);
  }
  cnd_.notify_all();
}

template <typename T> bool Channel<T>::closed() const noexcept {
  return is_closed_.load();
}
} // namespace libcomm