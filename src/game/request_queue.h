#ifndef __REQUEST_QUEUE_H__
#define __REQUEST_QUEUE_H__

#include <deque>
#include <functional>

#include "holders.h"

namespace banggame {

    template<typename Derived>
    struct request_queue {
        std::deque<request_holder> m_requests;
        std::deque<std::function<void()>> m_delayed_actions;

        request_holder &top_request() {
            return m_requests.front();
        }

        template<typename T = request_base>
        T *top_request_if(player *target = nullptr) {
            if (m_requests.empty()) return nullptr;
            request_holder &req = top_request();
            return !target || req.target() == target ? req.get_if<T>() : nullptr;
        }

        template<typename T>
        bool top_request_is(player *target = nullptr) {
            return top_request_if<T>(target) != nullptr;
        }
        
        void update_request() {
            static_cast<Derived &>(*this).send_request_update();
            while (m_requests.empty() && !m_delayed_actions.empty()) {
                std::invoke(m_delayed_actions.front());
                m_delayed_actions.pop_front();
            }
        }

        void queue_request_front(std::shared_ptr<request_base> &&value) {
            m_requests.emplace_front(std::move(value));
            update_request();
        }

        void queue_request(std::shared_ptr<request_base> &&value) {
            m_requests.emplace_back(std::move(value));
            if (m_requests.size() == 1) {
                update_request();
            }
        }

        template<std::derived_from<request_base> T, typename ... Ts>
        void queue_request_front(Ts && ... args) {
            queue_request_front(std::make_shared<T>(std::forward<Ts>(args) ... ));
        }
        
        template<std::derived_from<request_base> T, typename ... Ts>
        void queue_request(Ts && ... args) {
            queue_request(std::make_shared<T>(std::forward<Ts>(args) ... ));
        }

        template<typename T = request_base>
        bool pop_request_noupdate() {
            if (!top_request_is<T>()) return false;
            m_requests.pop_front();
            return true;
        }

        template<typename T = request_base>
        bool pop_request() {
            if (pop_request_noupdate<T>()) {
                update_request();
                return true;
            }
            return false;
        }

        template<std::invocable Function>
        int num_queued_requests(Function &&fun) {
            int nreqs = m_requests.size();
            std::invoke(std::forward<Function>(fun));
            return m_requests.size() - nreqs;
        }

        void tick() {
            if (!m_requests.empty()) {
                top_request().tick();
            }
        }

        template<std::invocable Function>
        void queue_action(Function &&fun) {
            if (m_requests.empty() && m_delayed_actions.empty()) {
                std::invoke(std::forward<Function>(fun));
            } else {
                m_delayed_actions.push_back(std::forward<Function>(fun));
            }
        }

        template<std::invocable Function>
        void queue_action_front(Function &&fun) {
            if (m_requests.empty() && m_delayed_actions.empty()) {
                std::invoke(std::forward<Function>(fun));
            } else {
                m_delayed_actions.push_front(std::forward<Function>(fun));
            }
        }
    };

}

#endif