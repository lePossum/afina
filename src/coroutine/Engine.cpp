#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <cstddef>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char stack_end;
    ctx.Hight = ctx.Low = StackBottom;
    if (&stack_end > StackBottom) {
        ctx.Hight = &stack_end;
    } else {
        ctx.Low = &stack_end;
    }

    char *&buffer = std::get<0>(ctx.Stack);
    uint32_t &av_size = std::get<1>(ctx.Stack);
    std::ptrdiff_t size = ctx.Hight - ctx.Low;

    if ((av_size < size) || ((av_size << 2 ) > size )) {
        if (buffer) {
            delete[] buffer;
        }
        av_size = size;
        buffer = new char[size];
    }

    memcpy(buffer, ctx.Low, size);
}

void Engine::Restore(context &ctx) {
    char stack_end;
    if (&stack_end >= ctx.Low && &stack_end <= ctx.Hight) {
        Restore(ctx);
    }

    char *&buffer = std::get<0>(ctx.Stack);
    std::ptrdiff_t size = ctx.Hight - ctx.Low;

    memcpy(ctx.Low, buffer, size);
    longjmp(ctx.Environment, 1);
}

void Engine::Enter(context &ctx) {
    if (cur_routine && cur_routine != idle_ctx) {
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }
        Store(*cur_routine);
    }

    cur_routine = &ctx;
    Restore(ctx);
}

void Engine::yield() {
    context *candidate = alive;

    if (candidate && candidate == cur_routine) {
        candidate = candidate->next;
    }

    if (candidate) {
        Enter(*candidate);
    }
}

void Engine::sched(void *routine_) {
    if (cur_routine == routine_) {
        return;
    }

    if (routine_) {
        Enter(*(static_cast<context *>(routine_)));
    } else {
        yield();
    }
}
} // namespace Coroutine
} // namespace Afina
