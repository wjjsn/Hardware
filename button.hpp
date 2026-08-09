#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <stdint.h>

namespace Hardware
{
using CallbackFunc = void (*)();

/*
 * 模板参数说明：
 * - GPIO: 传入的 GPIO 驱动类（需提供 init() 和 read() 方法）
 * - TrigState: 触发电平（false: 低电平触发 / 有效，true: 高电平触发 / 有效）
 * - HoldTask: 长按回调函数
 * - ClickTasks: 连击回调函数包（依次为 单击、双击、三击...）
 */
template <typename gpio, bool trigger_state, CallbackFunc hold_task, CallbackFunc... click_tasks>
struct StaticKey
{
private:
	// 编译期自动计算注册的最高连击次数
	static constexpr std::size_t max_clicks = sizeof...(click_tasks);

	// 静态状态变量
	inline static std::atomic<bool> current_state{!trigger_state};
	inline static bool previous_state = !trigger_state; // ISR-owned after init().
	inline static std::atomic<uint32_t> click_count{0};
	inline static uint32_t previous_click_count = 0; // Main-loop-owned.
	inline static std::atomic<uint32_t> hold_count{0};
	inline static std::atomic<bool> holding{false};

public:
	// 1. 初始化接口
	static void init()
	{
		gpio::init();
		const bool state = gpio::read();
		current_state.store(state, std::memory_order_relaxed);
		previous_state = state;
		click_count.store(0, std::memory_order_relaxed);
		previous_click_count = 0;
		hold_count.store(0, std::memory_order_relaxed);
		holding.store(false, std::memory_order_relaxed);
	}

	// 2. 检测长按状态
	static void detect_key_hold()
	{
		if (hold_count.load(std::memory_order_acquire) >= 60U &&
			!holding.exchange(true, std::memory_order_acq_rel))
		{
			hold_count.store(60, std::memory_order_release);
			if (hold_task)
			{
				hold_task();
			}
		}
	}

	// 3. 按键状态扫描（建议在定时器中断中调用，如 10ms 或 20ms）
	static void detect_key_click()
	{
		previous_state = current_state.load(std::memory_order_relaxed);
		const bool state = gpio::read();
		current_state.store(state, std::memory_order_release);

		// 检测释放边缘（从触发电平变为非触发电平，计为一次点击完成）
		if (state == !trigger_state && previous_state == trigger_state)
		{
			click_count.fetch_add(1U, std::memory_order_release);
		}

		// 持续处于触发状态，累加长按计数
		if (state == trigger_state && previous_state == trigger_state)
		{
			uint32_t count = hold_count.load(std::memory_order_relaxed);
			while (count < 60U && !hold_count.compare_exchange_weak(
				count, count + 1U, std::memory_order_release, std::memory_order_relaxed))
			{
			}
		}

		// 持续处于释放状态，清空长按计数
		if (state == !trigger_state && previous_state == !trigger_state)
		{
			hold_count.store(0, std::memory_order_release);
		}
	}

	// 4. 处理点击事件（在主循环中调用）
	static void cope_click_data()
	{
		// 长按判定 (从 detect_key_click 移到这里, 回调在主循环上下文执行)
		detect_key_hold();

		// 状态还在改变，或者没有点击，直接返回（消抖或等待连续点击结束）
		const uint32_t observed_clicks = click_count.load(std::memory_order_acquire);
		if (observed_clicks != previous_click_count || observed_clicks == 0U ||
			current_state.load(std::memory_order_acquire) == trigger_state)
		{
			previous_click_count = observed_clicks;
			return;
		}

		// 计数稳定且不为 0，说明连击结束，开始处理
		const uint32_t clicks = click_count.exchange(0U, std::memory_order_acq_rel);
		previous_click_count = 0;
		if (clicks != 0U)
		{
			const std::size_t click_index = clicks - 1U;

			// 将编译期参数包直接初始化为局部静态只读数组
			static constexpr std::array<CallbackFunc, max_clicks> tasks{click_tasks...};

			// O(1) 数组直达，安全检查边界后直接调用
			if (click_index < max_clicks)
			{
				if (click_index == 0 && holding.exchange(false, std::memory_order_acq_rel))
				{
					holding = false;
				}
				else if (tasks[click_index])
				{
					tasks[click_index]();
				}
			}

		}
	}
};
}
