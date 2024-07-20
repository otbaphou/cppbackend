#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

using OrderHandler = std::function<void(Result<HotDog> hot_dog)>;

class Order : public std::enable_shared_from_this<Order>
{
public:
    Order(net::io_context& io, std::shared_ptr<Sausage> sosig, std::shared_ptr<Bread> dough, std::shared_ptr<GasCooker> diablo, OrderHandler handler)
        : io_{ io },
        sausage_(sosig),
        bun_(dough),
        cooker_(diablo),
        strand_(net::make_strand(io)),
        handler_{ std::move(handler) } {}
    

    // Запускает асинхронное выполнение заказа
    void Execute()
    {
        net::dispatch(strand_, [self = shared_from_this()]()
            {
                self->GrillSausage();
                self->BakeBun();
            });
    }

private:

    void GrillSausage()
    {
        sausage_->StartFry(*cooker_, [self = shared_from_this()]()
            {
                self->sausage_timer_.async_wait([self](sys::error_code)
                    {
                        self->OnGrilled();
                    });
            });
    }

    void OnGrilled()
    {
        sausage_->StopFry();
        CheckReadiness();
    }

    void BakeBun()
    {
        bun_->StartBake(*cooker_, [self = shared_from_this()]()
            {
                self->bun_timer_.async_wait([self](sys::error_code)
                    {
                        self->OnBaked();
                    });
            });
    }

    void OnBaked()
    {
        bun_->StopBaking();
        CheckReadiness();
    }

    void CheckReadiness()
    {
        if (delivered_)
        {
            return;
        }

        if (sausage_->IsCooked() && bun_->IsCooked())
        {
            Deliver({});
        }
    }

    void Deliver(sys::error_code ec)
    {
        // Защита заказа от повторной доставки
        delivered_ = true;

        handler_(HotDog(++next_id_, sausage_, bun_));
    }

    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bun_;

    std::shared_ptr<GasCooker> cooker_;

    net::io_context& io_;
    net::strand <net::io_context::executor_type> strand_;
    std::function<void(Result<HotDog> hot_dog)> handler_;

    net::steady_timer sausage_timer_{ io_, HotDog::MIN_SAUSAGE_COOK_DURATION };
    net::steady_timer bun_timer_{ io_, HotDog::MIN_SAUSAGE_COOK_DURATION };
    int next_id_ = 0;
    bool delivered_ = false;
};

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Cafeteria 
{
public:
    explicit Cafeteria(net::io_context& io)
        : io_{ io } {
    }

    void OrderHotDog(HotDogHandler&& handler) 
    {
        std::make_shared<Order>(io_, store_.GetSausage(), store_.GetBread(), gas_cooker_, std::move(handler))->Execute();
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
}; 
