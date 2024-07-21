#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/bind_executor.hpp>
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
    Order(net::io_context& io, int id, std::shared_ptr<Sausage> sosig, std::shared_ptr<Bread> dough, std::shared_ptr<GasCooker> diablo, OrderHandler handler)
        : io_{ io },
        id_(id),
        sausage_(sosig),
        bun_(dough),
        cooker_(diablo),
        handler_{ std::move(handler) } {

        //std::cout << "PASSED BREAD WITH ID: " << dough->GetId() << '\n';
        //std::cout << "RECEIVED BREAD WITH ID: " << bun_->GetId() << '\n';

        //std::cout << "PASSED SOSIG WITH ID: " << sosig->GetId() << '\n';
        //std::cout << "RECEIVED SOSIG WITH ID: " << sausage_->GetId() << '\n';
    }
    

    // Запускает асинхронное выполнение заказа
    void Execute()
    {
        GrillSausage();
        BakeBun();
    }

private:

    void GrillSausage()
    {
        sausage_timer_.expires_after(HotDog::MIN_SAUSAGE_COOK_DURATION);

        sausage_->StartFry(*cooker_, net::bind_executor(strand_, [self = shared_from_this()]()
            {
                self->sausage_timer_.async_wait([self](sys::error_code ec)
                    {
                        self->OnGrilled(ec);
                    });
            }));
    }

    void OnGrilled(sys::error_code ec)
    {
        sausage_->StopFry();
        is_grilled_ = true;
        CheckReadiness(ec);
    }

    void BakeBun()
    {
        bun_timer_.expires_after(HotDog::MIN_BREAD_COOK_DURATION);

        bun_->StartBake(*cooker_, [self = shared_from_this()]()
            {
                self->bun_timer_.async_wait([self](sys::error_code ec)
                    {
                        self->OnBaked(ec);
                    });
            });
    }

    void OnBaked(sys::error_code ec)
    {
        bun_->StopBaking();
        is_baked_ = true;
        CheckReadiness(ec); 
    }

    void CheckReadiness(sys::error_code ec)
    {
        if (ec)
        {
            std::osyncstream os{ std::cout };
            os << ec.what() << " ERROR! ";
            return;
        }

        if(is_grilled_ && is_baked_)
        handler_(Result(std::move(HotDog(id_, sausage_, bun_))));
    }

    std::shared_ptr<Sausage> sausage_;
    std::shared_ptr<Bread> bun_;

    std::shared_ptr<GasCooker> cooker_;

    net::io_context& io_;
    net::strand <net::io_context::executor_type> strand_{ net::make_strand(io_) };
    std::function<void(Result<HotDog> hot_dog)> handler_;

    net::steady_timer sausage_timer_{ io_, HotDog::MIN_SAUSAGE_COOK_DURATION };
    net::steady_timer bun_timer_{ io_, HotDog::MIN_BREAD_COOK_DURATION };

    int id_;
    bool is_grilled_ = false, is_baked_ = false;
};

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Cafeteria 
{
public:
    explicit Cafeteria(net::io_context& io)
        : io_{ io } {
    }

    void OrderHotDog(int id, HotDogHandler handler) 
    {
        std::make_shared<Order>(io_, id, store_.GetSausage(), store_.GetBread(), gas_cooker_, std::move(handler))->Execute();
    }

private:
    net::io_context& io_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
}; 
