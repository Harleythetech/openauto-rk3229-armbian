/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace f1x
{
    namespace openauto
    {
        namespace autoapp
        {
            namespace projection
            {

                /**
                 * @brief Lock-free single-producer single-consumer (SPSC) ring buffer
                 *
                 * This buffer is designed for real-time audio where one thread produces
                 * data and another consumes it. No locks are used - only atomic operations
                 * with proper memory ordering for thread safety.
                 *
                 * @tparam Capacity Must be a power of 2 for efficient modulo via bitmask
                 */
                template <size_t Capacity>
                class LockFreeRingBuffer
                {
                    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
                    static_assert(Capacity > 0, "Capacity must be greater than 0");

                public:
                    LockFreeRingBuffer() : head_(0), tail_(0)
                    {
                        // Zero-initialize buffer
                        std::memset(buffer_, 0, Capacity);
                    }

                    /**
                     * @brief Write data to the buffer (producer side)
                     * @param data Pointer to data to write
                     * @param size Number of bytes to write
                     * @return Number of bytes actually written (may be less if buffer is full)
                     */
                    size_t write(const void *data, size_t size)
                    {
                        if (!data || size == 0)
                            return 0;

                        const size_t head = head_.load(std::memory_order_relaxed);
                        const size_t tail = tail_.load(std::memory_order_acquire);

                        // Calculate available space (leave one slot empty to distinguish full from empty)
                        const size_t available = (tail - head - 1 + Capacity) & mask_;
                        const size_t toWrite = std::min(size, available);

                        if (toWrite == 0)
                            return 0;

                        const uint8_t *src = static_cast<const uint8_t *>(data);
                        const size_t headIdx = head & mask_;

                        // Check if write wraps around
                        const size_t firstPart = std::min(toWrite, Capacity - headIdx);
                        std::memcpy(buffer_ + headIdx, src, firstPart);

                        if (toWrite > firstPart)
                        {
                            // Write wrapped portion at beginning
                            std::memcpy(buffer_, src + firstPart, toWrite - firstPart);
                        }

                        // Update head with release semantics so consumer sees the data
                        head_.store(head + toWrite, std::memory_order_release);

                        return toWrite;
                    }

                    /**
                     * @brief Read data from the buffer (consumer side)
                     * @param data Pointer to buffer to read into
                     * @param size Maximum number of bytes to read
                     * @return Number of bytes actually read
                     */
                    size_t read(void *data, size_t size)
                    {
                        if (!data || size == 0)
                            return 0;

                        const size_t tail = tail_.load(std::memory_order_relaxed);
                        const size_t head = head_.load(std::memory_order_acquire);

                        // Calculate available data
                        const size_t available = (head - tail) & mask_;
                        const size_t toRead = std::min(size, available);

                        if (toRead == 0)
                            return 0;

                        uint8_t *dst = static_cast<uint8_t *>(data);
                        const size_t tailIdx = tail & mask_;

                        // Check if read wraps around
                        const size_t firstPart = std::min(toRead, Capacity - tailIdx);
                        std::memcpy(dst, buffer_ + tailIdx, firstPart);

                        if (toRead > firstPart)
                        {
                            // Read wrapped portion from beginning
                            std::memcpy(dst + firstPart, buffer_, toRead - firstPart);
                        }

                        // Update tail with release semantics
                        tail_.store(tail + toRead, std::memory_order_release);

                        return toRead;
                    }

                    /**
                     * @brief Get number of bytes available to read
                     */
                    size_t available() const
                    {
                        const size_t head = head_.load(std::memory_order_acquire);
                        const size_t tail = tail_.load(std::memory_order_relaxed);
                        return (head - tail) & mask_;
                    }

                    /**
                     * @brief Get number of bytes available for writing
                     */
                    size_t space() const
                    {
                        const size_t head = head_.load(std::memory_order_relaxed);
                        const size_t tail = tail_.load(std::memory_order_acquire);
                        return (tail - head - 1 + Capacity) & mask_;
                    }

                    /**
                     * @brief Clear the buffer
                     * @note Only safe to call when no reads/writes are in progress
                     */
                    void clear()
                    {
                        head_.store(0, std::memory_order_relaxed);
                        tail_.store(0, std::memory_order_relaxed);
                    }

                    /**
                     * @brief Get the total capacity
                     */
                    static constexpr size_t capacity() { return Capacity - 1; }

                private:
                    static constexpr size_t mask_ = Capacity - 1;

                    alignas(64) std::atomic<size_t> head_; // Written by producer
                    alignas(64) std::atomic<size_t> tail_; // Written by consumer
                    alignas(64) uint8_t buffer_[Capacity];
                };

            } // namespace projection
        } // namespace autoapp
    } // namespace openauto
} // namespace f1x
