#pragma once
// https://github.com/andreiavrammsd/cpp-channel

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
  bool operator!=(BlockingIterator<Channel>) const {
    std::unique_lock<std::mutex> lock{chan_.mtx_};
    chan_.waitBeforeRead(lock);
    return !(chan_.closed() && chan_.empty());
  }

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

/**
 * @brief Exception thrown if trying to write on closed channel.
 */
class ClosedChannel : public std::runtime_error {
public:
  explicit ClosedChannel(const char *msg) : std::runtime_error{msg} {}
};

/**
 * @brief Thread-safe container for sharing data between threads.
 *
 * Implements a blocking input iterator.
 *
 * @tparam T The type of the elements.
 */
template <typename T> class Channel {
public:
  using value_type = T;
  using iterator = BlockingIterator<Channel<T>>;
  using size_type = std::size_t;

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
  NODISCARD inline size_type constexpr size() const noexcept;

  /**
   * Returns true if there are no elements in channel.
   */
  NODISCARD inline bool constexpr empty() const noexcept;

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
  iterator begin() noexcept;
  iterator end() noexcept;

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
  std::atomic<std::size_t> size_{0};
  std::mutex mtx_;
  std::condition_variable cnd_;
  std::atomic<bool> is_closed_{false};

  void waitBeforeRead(std::unique_lock<std::mutex> &lock) {
    cnd_.wait(lock, [this]() { return !empty() || closed(); });
  }
  void waitBeforeWrite(std::unique_lock<std::mutex> &lock) {
    cnd_.wait(lock, [this]() { return size_ <= cap_; });
  }
  friend class BlockingIterator<Channel>;
};

template <typename T>
constexpr Channel<T>::Channel(const size_type capacity) : cap_{capacity} {}

template <typename T>
Channel<typename std::decay<T>::type> &
operator<<(Channel<typename std::decay<T>::type> &ch, T &&in) {
  if (ch.closed()) {
    throw ClosedChannel{"cannot write on closed channel"};
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

  {
    std::unique_lock<std::mutex> lock{ch.mtx_};
    ch.waitBeforeRead(lock);
    if (!ch.empty()) {
      out = std::move(ch.queue_.front());
      ch.queue_.pop();
      --ch.size_;
    }
  }

  ch.cnd_.notify_one();

  return ch;
}

template <typename T>
constexpr typename Channel<T>::size_type Channel<T>::size() const noexcept {
  return size_;
}

template <typename T> constexpr bool Channel<T>::empty() const noexcept {
  return size_ == 0;
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

template <typename T>
BlockingIterator<Channel<T>> Channel<T>::begin() noexcept {
  return BlockingIterator<Channel<T>>{*this};
}

template <typename T> BlockingIterator<Channel<T>> Channel<T>::end() noexcept {
  return BlockingIterator<Channel<T>>{*this};
}
} // namespace libcomm