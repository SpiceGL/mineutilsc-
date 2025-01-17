﻿//mineutils库的便利时间相关工具
//注：如果为qnx6.6平台编译，可能需要为编译器开启宏: _GLIBCXX_USE_NANOSLEEP
#pragma once
#ifndef TIME_HPP_MINEUTILS
#define TIME_HPP_MINEUTILS

#include<chrono>
#include<map>
#include<stdio.h>
#include<string>
#include<thread>
#include<vector>

#include"base.hpp"

namespace mineutils
{
    /*--------------------------------------------用户接口--------------------------------------------*/

    //基于<chrono>库的简易计时函数封装
    namespace mtime
    {
        using TimePoint = std::chrono::steady_clock::time_point;   //时间点
        using Duration = decltype(TimePoint() - TimePoint());   //时间段

        //时间单位
        enum class Unit
        {
            s = 0,
            ms = 1,
            us = 2,
            ns = 3
        };

        //获取当前时间点
        TimePoint now();

        //将“时间段(mtime::Duration)”类型转化为以秒为单位的数字
        long long s(const mtime::Duration& t);

        //将“时间段(mtime::Duration)”类型转化为以毫秒为单位的数字
        long long ms(const mtime::Duration& t);

        //将“时间段(mtime::Duration)”类型转化为以微秒为单位的数字
        long long us(const mtime::Duration& t);

        //将“时间段(mtime::Duration)”类型转化为以纳秒为单位的数字
        long long ns(const mtime::Duration& t);

        //进程休眠(秒)
        void sleep(long long t);

        //进程休眠(毫秒)
        void msleep(long long t);

        //进程休眠(微秒)
        void usleep(long long t);

        //进程休眠(纳秒)
        void nsleep(long long t);

        //只有开启时TimeCounter系列类的统计功能才生效，默认为开启状态
        void setGlobalTimeCounterOn(bool glob_timecounter_on);


        //用于统计各个代码段的在一定循环次数的平均消耗时间，非线程安全
        //调用addStart和addEnd会带来少量时间损耗(在rv1126上约为12微秒)
        class MeanTimeCounter
        {
        private:
            class Guard;

        public:
            MeanTimeCounter() {}
            /*  构造MeanTimeCounter类
                @param target_count_times: 每轮统计次数，小于1的值会被置为1
                @param time_counter_on: 计时功能开关，为false会跳过计时功能   */
            explicit MeanTimeCounter(int target_count_times, bool time_counter_on = true);

            /*  本轮统计开始，应在目标统计代码段前调用，与段后addEnd成对出现
                @param codeblock_tag: 要统计的代码段的tag   */
            void addStart(const std::string& codeblock_tag);

            /*  本轮统计结束，应在目标统计代码段后调用，与段前addStart成对出现
                @param codeblock_tag: 要统计的代码段的tag   */
            void addEnd(const std::string& codeblock_tag);

            /*  使用RAII方式记录一段代码的耗时
                - 用法：auto guard = time_counter.addGuard("codeblock_tag")
                @param codeblock_tag: 要统计的代码段的tag
                @return 一个私有类Guard对象，只能用auto推导；在返回时记录开始时间，在被析构时记录结束时间  */
            MeanTimeCounter::Guard addGuard(std::string codeblock_tag);

            /*  在codeblock_tag代码段达到目标统计次数后输出平均消耗时间，并重新开始统计此段代码
                @param codeblock_tag: 输出信息中被统计代码段的tag
                @param time_unit: 输出信息中时间统计的单位，输入强枚举类型mtime::Unit的成员
                @return 若达到目标统计次数，则按time_unit返回平均耗时；否则返回-1   */
            long long printMeanTimeCost(const std::string& codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms);

            /*  在codeblock_tag代码段达到目标统计次数后输出平均消耗时间，并重新开始统计此段代码
                @param print_head: 输出信息的头部内容，推荐输入调用printMeanTimeCost的函数的名字
                @param codeblock_tag: 输出信息中被统计代码段的tag
                @param time_unit: 输出信息中时间统计的单位，输入强枚举类型mtime::Unit的成员
                @return 若达到目标统计次数，则按time_unit返回平均耗时；否则返回-1   */
            long long printMeanTimeCost(const std::string& print_head, const std::string& codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms);

            /*  在每个被统计的代码段达到目标统计次数后，输出其平均消耗时间并重新开始统计此段代码
                @param time_unit: 输出信息中时间统计的单位，输入强枚举类型mtime::Unit的成员   */
            void printAllMeanTimeCost(mtime::Unit time_unit = mtime::Unit::ms);

            /*  在每个被统计的代码段达到目标统计次数后，输出其平均消耗时间并重新开始统计此段代码
                @param print_head: 输出信息的头部内容，推荐输入调用printAllMeanTimeCost的函数的名字
                @param time_unit: 输出信息中时间统计的单位，输入强枚举类型mtime::Unit的成员   */
            void printAllMeanTimeCost(const std::string& print_head, mtime::Unit time_unit = mtime::Unit::ms);

        private:
            class SingleCounter;

            int target_count_times_ = 1;
            std::map<std::string, SingleCounter> time_counter_;
            std::vector<std::string> keys_;
            bool time_counter_on_ = true;

        public:
            mdeprecated(R"(Deprecated. Please replace with function "MeanTimeCounter::addGuard"(in time.hpp) )") Guard addLocal(std::string codeblock_tag);
        };


        //统计代码块的运行时间，在创建对象时开始计时，在析构时停止计时并打印耗时
        class TimeCounterGuard
        {
        public:
            /*  构造TimeCounterGuard类
                @param codeblock_tag: 要计时的代码块标识符
                @param time_unit: 计时单位，强枚举类型mtime::Unit的成员，默认为ms
                @param time_counter_on: 计时功能开关，为false会跳过计时功能   */
            TimeCounterGuard(const std::string& codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms, bool time_counter_on = true);

            /*  构造TimeCounterGuard类
                @param print_head: 输出信息的头部内容，比如使用创建该类对象的函数名
                @param codeblock_tag: 要计时的代码块标识符
                @param time_unit: 计时单位，强枚举类型mtime::Unit的成员，默认为ms
                @param time_counter_on: 是否开启计时功能，默认为true   */
            TimeCounterGuard(const std::string& print_head, const std::string& codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms, bool time_counter_on = true);

            TimeCounterGuard(const TimeCounterGuard& _temp) = delete;
            TimeCounterGuard(TimeCounterGuard&& _temp) = delete;
            TimeCounterGuard& operator=(const TimeCounterGuard& _temp) = delete;
            TimeCounterGuard& operator=(TimeCounterGuard&& _temp) = delete;
            ~TimeCounterGuard();

        private:
            bool time_counter_on_;
            mtime::TimePoint start_t_;
            mtime::TimePoint end_t_;
            std::string codeblock_tag_;
            mtime::Unit time_unit_;
        };

        //控制代码段的时间消耗不低于设定时间，单位越小精度越高
        class TimeControllerGuard
        {
        public:
            TimeControllerGuard(long long target_time, mtime::Unit time_unit = mtime::Unit::ms);

            TimeControllerGuard(const TimeControllerGuard& _temp) = delete;
            TimeControllerGuard(TimeControllerGuard&& _temp) = delete;
            TimeControllerGuard& operator=(const TimeControllerGuard& _temp) = delete;
            TimeControllerGuard& operator=(TimeControllerGuard&& _temp) = delete;
            ~TimeControllerGuard();

        private:
            mtime::TimePoint start_t_;
            mtime::TimePoint end_t_;
            long long target_time_;
            mtime::Unit time_unit_;
        };
    }








    /*--------------------------------------------内部实现--------------------------------------------*/

    namespace mtime
    {
        //获取当前时间点(mtime::time_point)
        inline TimePoint now()
        {
            return std::chrono::steady_clock::now();
        }

        //inline long long _countTime(const mtime::Duration& t, mtime::Unit unit)
        //{
        //    if (unit == mtime::Unit::s)
        //        return std::chrono::duration_cast<std::chrono::seconds>(t).count();
        //    else if (unit == mtime::Unit::ms)
        //        return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
        //    else if (unit == mtime::Unit::us)
        //        return std::chrono::duration_cast<std::chrono::microseconds>(t).count();
        //    else if (unit == mtime::Unit::ns)
        //        return std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();
        //}

        //将“时间段(mtime::Duration)”类型转化为以秒为单位的数字
        inline long long s(const mtime::Duration& t)
        {
            return std::chrono::duration_cast<std::chrono::seconds>(t).count();
        }

        //将“时间段(mtime::Duration)”类型转化为以毫秒为单位的数字
        inline long long ms(const mtime::Duration& t)
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
        }

        //将“时间段(mtime::Duration)”类型转化为以微秒为单位的数字
        inline long long us(const mtime::Duration& t)
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(t).count();
        }

        //将“时间段(mtime::Duration)”类型转化为以纳秒为单位的数字
        inline long long ns(const mtime::Duration& t)
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(t).count();
        }

        //进程休眠(秒)
        inline void sleep(long long t)
        {
            if (t > 0)
                std::this_thread::sleep_for(std::chrono::seconds(t));
        }

        //进程休眠(毫秒)
        inline void msleep(long long t)
        {
            if (t > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(t));
        }

        //进程休眠(微秒)
        inline void usleep(long long t)
        {
            if (t > 0)
                std::this_thread::sleep_for(std::chrono::microseconds(t));
        }

        //进程休眠(纳秒)
        inline void nsleep(long long t)
        {
            if (t > 0)
                std::this_thread::sleep_for(std::chrono::nanoseconds(t));
        }


        inline bool& _getTimeCounterOn()
        {
            static bool g_timecounter_on = true;
            return g_timecounter_on;
        }

        //设置后所有TimeCounter系列类的功能将被跳过
        inline void setGlobalTimeCounterOn(bool glob_timecounter_on)
        {
            bool& timecounter_on = mtime::_getTimeCounterOn();
            timecounter_on = glob_timecounter_on;
        }


        class MeanTimeCounter::Guard
        {
        public:
            Guard(Guard&& tmp) noexcept
            {
                this->codeblock_tag_ = std::move(tmp.codeblock_tag_);
                this->self_ = tmp.self_;
                tmp.self_ = nullptr;
            }

            ~Guard()
            {
                if (this->self_)
                    this->self_->addEnd(this->codeblock_tag_);
            }

            Guard(const Guard& tmp) = delete;
            Guard& operator=(const Guard& tmp) = delete;
            Guard& operator=(Guard&& tmp) = delete;

        private:
            Guard(MeanTimeCounter* self, std::string& codeblock_tag)
            {
                self->addStart(codeblock_tag);
                this->self_ = self;
                this->codeblock_tag_ = std::move(codeblock_tag);
            }

            MeanTimeCounter* self_ = nullptr;
            std::string codeblock_tag_;
            friend MeanTimeCounter;
        };

        class MeanTimeCounter::SingleCounter
        {
        public:
            SingleCounter() = default;

            explicit SingleCounter(int target_count_times)
            {
                this->target_count_times_ = target_count_times >= 1 ? target_count_times : 1;
            }

            void addStart()
            {
                this->start_t_ = mtime::now();
                this->addstart_times_ += 1;
            }

            void addEnd()
            {
                this->end_t_ = mtime::now();
                this->time_cost_ += (this->end_t_ - this->start_t_);
                this->addend_times_ += 1;
                this->now_statistical_times_ += 1;                
            }

            long long printMeanTimeCost(const std::string& codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms)
            {
                if (!(this->addend_times_ == this->addstart_times_))
                {
                    printf("!Warning!: MeanTimeCounter::%s: Function \'addStart()\' and function \'addEnd()\' should be called the same number of times before function \'%s(%s)\'!\n", __func__, __func__, codeblock_tag.c_str());
                    return -1;
                }
                if (this->finish())
                {
                    long long mean_time_cost;
                    if (time_unit == mtime::Unit::s)
                    {
                        mean_time_cost = mtime::s(this->time_cost_) / this->now_statistical_times_;
                        printf("%s mean cost time %llds in %d counts\n", codeblock_tag.c_str(), mean_time_cost, this->now_statistical_times_);
                    }
                    else if (time_unit == mtime::Unit::ms)
                    {
                        mean_time_cost = mtime::ms(this->time_cost_) / this->now_statistical_times_;
                        printf("%s mean cost time %lldms in %d counts\n", codeblock_tag.c_str(), mean_time_cost, this->now_statistical_times_);
                    }
                    else if (time_unit == mtime::Unit::us)
                    {
                        mean_time_cost = mtime::us(this->time_cost_) / this->now_statistical_times_;
                        printf("%s mean cost time %lldus in %d counts\n", codeblock_tag.c_str(), mean_time_cost, this->now_statistical_times_);
                    }
                    else if (time_unit == mtime::Unit::ns)
                    {
                        mean_time_cost = mtime::ns(this->time_cost_) / this->now_statistical_times_;
                        printf("%s mean cost time %lldns in %d counts\n", codeblock_tag.c_str(), mean_time_cost, this->now_statistical_times_);
                    }
                    else
                    {
                        mean_time_cost = mtime::ms(this->time_cost_) / this->now_statistical_times_;
                        printf("%s mean cost time %lldms in %d counts\n", codeblock_tag.c_str(), mean_time_cost, this->now_statistical_times_);
                    }
                    this->restart();
                    return mean_time_cost;
                }
                else return -1;
            }

            long long printMeanTimeCost(const std::string& print_head, const std::string& codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms)
            {
                return this->printMeanTimeCost("\"" + print_head + "\": " + codeblock_tag, time_unit);
            }

        private:
            bool finish()
            {
                if (this->now_statistical_times_ >= this->target_count_times_)
                    return true;
                else return false;
            }

            void restart()
            {
                this->now_statistical_times_ = 0;
                this->addstart_times_ = 0;
                this->addend_times_ = 0;
                this->time_cost_ = mtime::Duration(0);
            }

            int now_statistical_times_ = 0;
            int target_count_times_ = 1;
            int addstart_times_ = 0;
            int addend_times_ = 0;

            mtime::Duration time_cost_ = mtime::Duration(0);
            mtime::TimePoint start_t_;
            mtime::TimePoint end_t_;
        };


        inline MeanTimeCounter::MeanTimeCounter(int target_count_times, bool time_counter_on)
        {
            this->target_count_times_ = target_count_times >= 1 ? target_count_times : 1;
            this->time_counter_on_ = time_counter_on;
        }

        inline void MeanTimeCounter::addStart(const std::string& codeblock_tag)
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
            {
                if (this->time_counter_.end() == this->time_counter_.find(codeblock_tag))
                {
                    this->time_counter_[codeblock_tag] = MeanTimeCounter::SingleCounter(this->target_count_times_);
                    this->keys_.push_back(codeblock_tag);
                }
                this->time_counter_[codeblock_tag].addStart();
            }
        }

        inline void MeanTimeCounter::addEnd(const std::string& codeblock_tag)
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
            {
                if (this->time_counter_.end() == this->time_counter_.find(codeblock_tag))
                {
                    printf("!!!Error!!! MeanTimeCounter::%s: Please call \"addStart(%s)\" before \"addEnd(%s)\"!\n", __func__, codeblock_tag.c_str(), codeblock_tag.c_str());
                    return;
                }
                this->time_counter_[codeblock_tag].addEnd();
            }
        }

        inline MeanTimeCounter::Guard MeanTimeCounter::addGuard(std::string codeblock_tag)
        {            
            return MeanTimeCounter::Guard(this, codeblock_tag);
        }

        inline long long MeanTimeCounter::printMeanTimeCost(const std::string& codeblock_tag, mtime::Unit time_unit)
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
                return this->time_counter_[codeblock_tag].printMeanTimeCost(codeblock_tag, time_unit);
            return -1;
        }

        inline long long MeanTimeCounter::printMeanTimeCost(const std::string& print_head, const std::string& codeblock_tag, mtime::Unit time_unit)
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
                return this->time_counter_[codeblock_tag].printMeanTimeCost(print_head, codeblock_tag, time_unit);
            return -1;
        }

        inline void MeanTimeCounter::printAllMeanTimeCost(mtime::Unit time_unit)
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
            {
                for (const std::string& codeblock_tag : this->keys_)
                {
                    this->time_counter_[codeblock_tag].printMeanTimeCost(codeblock_tag, time_unit);
                }
            }
        }

        inline void MeanTimeCounter::printAllMeanTimeCost(const std::string& print_head, mtime::Unit time_unit)
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
            {
                for (const std::string& codeblock_tag : this->keys_)
                {
                    this->time_counter_[codeblock_tag].printMeanTimeCost(print_head, codeblock_tag, time_unit);
                }
            }
        }

        inline MeanTimeCounter::Guard MeanTimeCounter::addLocal(std::string codeblock_tag)
        {
            return this->addGuard(std::move(codeblock_tag));
        }

        inline TimeCounterGuard::TimeCounterGuard(const std::string& codeblock_tag, mtime::Unit time_unit, bool time_counter_on)
        {
            this->start_t_ = mtime::now();
            this->codeblock_tag_ = codeblock_tag;
            this->time_unit_ = time_unit;
            this->time_counter_on_ = time_counter_on;
        }

        inline TimeCounterGuard::TimeCounterGuard(const std::string& print_head, const std::string& codeblock_tag, mtime::Unit time_unit, bool time_counter_on)
        {
            this->start_t_ = mtime::now();
            this->codeblock_tag_ = "\"" + print_head + "\": " + codeblock_tag;
            this->time_unit_ = time_unit;
            this->time_counter_on_ = time_counter_on;
        }

        inline TimeCounterGuard::~TimeCounterGuard()
        {
            if (mtime::_getTimeCounterOn() && this->time_counter_on_)
            {
                this->end_t_ = mtime::now();
                if (this->time_unit_ == mtime::Unit::s)
                    printf("%s cost time %llds\n", this->codeblock_tag_.c_str(), mtime::s(this->end_t_ - this->start_t_));
                else if (this->time_unit_ == mtime::Unit::ms)
                    printf("%s cost time %lldms\n", this->codeblock_tag_.c_str(), mtime::ms(this->end_t_ - this->start_t_));
                else if (this->time_unit_ == mtime::Unit::us)
                    printf("%s cost time %lldus\n", this->codeblock_tag_.c_str(), mtime::us(this->end_t_ - this->start_t_));
                else if (this->time_unit_ == mtime::Unit::ns)
                    printf("%s cost time %lldns\n", this->codeblock_tag_.c_str(), mtime::ns(this->end_t_ - this->start_t_));
                else
                    printf("%s cost time %lldms\n", this->codeblock_tag_.c_str(), mtime::ms(this->end_t_ - this->start_t_));
            }
        }

        inline TimeControllerGuard::TimeControllerGuard(long long target_time, mtime::Unit time_unit)
        {
            this->start_t_ = mtime::now();
            this->target_time_ = target_time;
            this->time_unit_ = time_unit;
        }

        inline TimeControllerGuard::~TimeControllerGuard()
        {
            this->end_t_ = mtime::now();
            if (this->time_unit_ == mtime::Unit::s)
            {
                long long used_time = mtime::s(this->end_t_ - this->start_t_);
                long long need_sleep = this->target_time_ - used_time;
                mtime::sleep(need_sleep);
            }
            else if (this->time_unit_ == mtime::Unit::ms)
            {
                long long used_time = mtime::ms(this->end_t_ - this->start_t_);
                long long need_sleep = this->target_time_ - used_time;
                mtime::msleep(need_sleep);
            }
            else if (this->time_unit_ == mtime::Unit::us)
            {
                long long used_time = mtime::us(this->end_t_ - this->start_t_);
                long long need_sleep = this->target_time_ - used_time;
                mtime::usleep(need_sleep);
            }
            else if (this->time_unit_ == mtime::Unit::ns)
            {
                long long used_time = mtime::ns(this->end_t_ - this->start_t_);
                long long need_sleep = this->target_time_ - used_time;
                mtime::nsleep(need_sleep);
            }
        }



        //已废弃
        class mdeprecated(R"(Deprecated. Please replace with class "MeanTimeCounter"(in time.hpp) )") MultiMeanTimeCounter :public MeanTimeCounter
        {
        public:
            MultiMeanTimeCounter() :MeanTimeCounter() {}
            explicit MultiMeanTimeCounter(int target_count_times, bool time_counter_on = true) :MeanTimeCounter(target_count_times, time_counter_on) {}
        };

        //已废弃
        class mdeprecated(R"(Deprecated. Please replace with class "TimeCounterGuard"(in time.hpp) )") LocalTimeCounter :public TimeCounterGuard
        {
        public:
            LocalTimeCounter(const std::string & codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms, bool time_counter_on = true) :TimeCounterGuard(codeblock_tag, time_unit, time_counter_on) {}
            LocalTimeCounter(const std::string & print_head, const std::string & codeblock_tag, mtime::Unit time_unit = mtime::Unit::ms, bool time_counter_on = true) :TimeCounterGuard(print_head, codeblock_tag, time_unit, time_counter_on) {}
        };

        //已废弃
        class mdeprecated(R"(Deprecated. Please replace with class "TimeControllerGuard"(in time.hpp) )") LocalTimeController :public TimeControllerGuard
        {
        public:
            LocalTimeController(long long target_time, mtime::Unit time_unit = mtime::Unit::ms) :TimeControllerGuard(target_time, time_unit) {}
        };
    }
}

#endif // !TIME_HPP_MINEUTILS