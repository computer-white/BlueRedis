# 协程句柄生命周期与 `ResumeTicket`

本文说明 Blue 框架里 **`std::coroutine_handle` 为什么不能直接交给 `IOManager`/定时器**，以及 `ResumeTicket` 如何把「是否还能 resume」的判断从协程帧内转移到帧外。这是框架里最容易被忽略、也最容易埋 UAF 的一层。

---

## 1. 问题的起点：`coroutine_handle` 是非拥有句柄

`std::coroutine_handle` 本质是一个裸指针，标准只保证：

> 协程处于挂起状态且不在最终挂起点时，`resume()` 有效。

帧一旦被 `destroy()`，`resume()` / `done()` / `promise()` 全是未定义行为。而 `IOManager::FdContext` 和 `Timer` 持有的正是这种裸句柄——**它们无法感知帧是否已经销毁**。

在 Blue 里，帧的销毁时机由「父帧里的 `Task<T>` 临时对象」决定：子协程 `co_return` 后，`co_await` 表达式结束，`Task<T>` 析构，`destroySafe()` 销毁子协程帧。`IOManager` / 定时器手里的句柄在同一时刻变成悬垂指针。

---

## 2. 为什么「帧内标志」是自指悖论

一个自然的想法是在 `promise_type` 里放 `std::atomic<bool> resumed`，resume 前 `exchange(true)` 防重复。但读写 `h.promise().resumed` **本身就要解引用 `h`**。帧销毁后，这一步就是 UAF。

**帧内的标志保护不了帧本身。** 判断「能不能 resume」的前提是「帧还活着」，而帧是否活着恰恰是你要判断的东西。

---

## 3. 为什么 `shared_ptr<TaskBase>` 不够

顶层 `Task` 经 `Scheduler::schedule(Task<T>)` 里的 `makeTaskBase` 包装成 `TaskModel<T>`，有 `shared_ptr<TaskBase>`。但**子协程没有**：

- `read_helper` 这类子协程是 `co_await` 表达式产生的临时 `Task<T>`，没有经过 `makeTaskBase`，构造不出 `shared_ptr<TaskBase>`；
- 用父协程的 `self` 保活子协程帧会破坏对称转移：`resume` 到父协程 `TaskModel` 而不是子协程句柄，`co_await` 的 `await_resume` 读不到子协程结果，语义错位。

所以「让每个协程都有 `shared_ptr`」这条路走不通。

---

## 4. `ResumeTicket`：把判断权转移到帧外

`ResumeTicket` 是一个**帧外对象**，只做一件事：原子地决定「谁有资格 resume」。

```cpp
struct ResumeTicket
{
    std::coroutine_handle<> m_handle;   // 直接 co_await AsyncIo 的协程，resume 目标
    std::atomic<bool> m_armed{true};    // 帧外判断权：是否还允许 resume
};
```
**并加入try_resume(const std::shared_ptr<ResumeTicket> &rt);函数**，无需增加额外schedule函数，只需包装成`lambda`复用函数即可

## 5. `ResumeTicket`：为什么安全？

1. `armed` 在帧外：帧销毁不影响 `ticket` 对象，判断本身不会 UAF；

2. `exchange` 返回 true 时帧一定活着：`exchange` 返回 true 说明 await_resume 的 store(false) 还没执行，即子协程还没恢复、还挂在 co_await AsyncIo 上，父帧里的 Task<T> 还没进入析构路径。此时 handle.done() / handle.resume() 安全；

3. `exchange` 返回 false 时完全不碰 handle：子协程已恢复或帧已销毁，`handle` 可能悬垂，但代码不访问它。

4. `resume` 之后 `lambda` 不再碰 handle，子协程 co_return 后 Task<T> 析构帧，发生在 handle 访问之后，没有竞态。

## 6. 边界与约束

1. `resume` 目标永远是「直接 `co_await AsyncIo` 的协程」的 `handle`，不是父协程的 `TaskModel`。父协程的 `shared_ptr` 只负责保活链（父帧 → Task<T> → 子帧），不负责 `resume`；

2. 帧保活仍由父帧里的 `Task<T>` 承担，`ticket` 只保证「访问 handle 时帧还没被 Task<T> 析构」，不延长帧生命周期；

3. 对称转移不受影响：子协程 co_return 后对称转移到 fa（父协程），语义不变；

4. 对调用方透明：Read / AcceptT 等工厂函数签名不变，`ResumeTicket 是 AsyncIo 内部实现细节`。

## 7. 如何新增一个异步IO

1. 新操作返回 AsyncIo`（参考 asyncio.h 里的 Read / AcceptT）`；

2. `await_suspend` 里 auto ticket = std::make_shared<ResumeTicket>(); ticket->m_handle = h; ticket->m_armed.store(true)；

3. `IOManager::addEvent(fd, event, ticket)`，带超时的再 `addTimer(ms, ..., [ticket]{ try_resume(ticket); })；`

4. 不要把`裸 coroutine_handle 在await_resume中直接同时交给 IOManager 和 定时器`，可能会导致 `UAF`。