#pragma once

#include <functional>
#include "game_server.hpp"
#include "network/address.hpp"
#include "utils/concurrency.hpp"

template <typename T>
class network_list
{
public:
	class iteration_context
	{
	public:
		friend network_list;

		const network::address& get_address() const { return *this->address_; }
		T& get() { return *this->element_; }
		const T& get() const { return *this->element_; }
		void remove() { this->remove_ = true; }
		void stop_iterating() const { this->stop_iterating_ = true; }
	
	private:
		T* element_{};
		const network::address* address_{};
		bool remove_{false};
		mutable bool stop_iterating_{false};
	};
	
	using iterate_func = std::function<void (iteration_context&)>;
	using const_iterate_func = std::function<void (const iteration_context&)>;
	
	using access_func = std::function<void(T&, const network::address&)>;
	using const_access_func = std::function<void(const T&, const network::address&)>;

	using insert_func = std::function<void(T&)>;

	bool find(const network::address& address, const access_func& accessor)
	{
		return this->elements_.template access<bool>([&](list_type& list)
		{
			const auto i = list.find(address);
			if(i == list.end())
			{
				return false;
			}

			accessor(i->second, i->first);
			return true;
		});
	}
	
	bool find(const network::address& address, const const_access_func& accessor) const
	{
		return this->elements_.template access<bool>([&](const list_type& list)
		{
			const auto i = list.find(address);
			if(i == list.end())
			{
				return false;
			}

			accessor(i->second, i->first);
			return true;
		});
	}

	void iterate(const iterate_func& iterator)
	{
		this->elements_.access([&](list_type& list)
		{
			iteration_context context{};
			
			for(auto i = list.begin(); i != list.end();)
			{
				context.element_ = &i->second;
				context.address_ = &i->first;
				context.remove_ = false;

				iterator(context);
				
				if(context.remove_)
				{
					i = list.erase(i);
				}
				else
				{
					++i;
				}

				if(context.stop_iterating_)
				{
					break;
				}
			}
		});
	}
	
	void iterate(const const_iterate_func& iterator) const
	{
		this->elements_.access([&](const list_type& list)
		{
			iteration_context context{};
			
			for(const auto& server : list)
			{
				// Const cast is ok here
				context.element_ = const_cast<T*>(&server.second);
				context.address_ = &server.first;
				
				iterator(context);

				if(context.stop_iterating_)
				{
					break;
				}
			}
		});
	}

protected:
	void insert(const network::address& address, const insert_func& callback)
	{
		this->elements_.access([&](list_type& list)
		{
			callback(list[address]);
		});
	}
	
private:
	using list_type = std::unordered_map<network::address, T>;
	utils::concurrency::container<list_type> elements_;
};