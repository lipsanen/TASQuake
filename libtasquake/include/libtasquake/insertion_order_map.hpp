#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace TASQuake {

    template<typename T>
	struct ordered_map {
		std::vector<std::pair<std::string, T>> values;

        T& operator[](const std::string& str) {
            for(std::pair<std::string, T>& value : values) {
                if(value.first == str) {
                    return value.second;
                }
            }

            values.push_back(std::pair<std::string, T>());
            values[values.size() - 1].first = str;
            return values[values.size() - 1].second;
        }

        const T& operator[](const std::string& str) const {
            for(const std::pair<std::string, T>& value : values) {
                if(value.first == str) {
                    return value.second;
                }
            }

            throw std::exception();
        }

        T& at(const std::string& str) {
            return operator[](str);
        }

        const T& at(const std::string& str) const {
            return operator[](str);
        }

        typename std::vector<std::pair<std::string, T>>::iterator begin() {
            return values.begin();
        }

        typename std::vector<std::pair<std::string, T>>::iterator end() {
            return values.end();
        }

        typename std::vector<std::pair<std::string, T>>::const_iterator begin() const {
            return values.begin();
        }

        typename std::vector<std::pair<std::string, T>>::const_iterator end() const {
            return values.end();
        }

        bool empty() const {
            return values.empty();
        }

        void clear() {
            return values.clear();
        }

        typename std::vector<std::pair<std::string, T>>::const_iterator find(const std::string& str) const {
            auto it = values.begin();
            for(it=values.begin(); it != values.end(); ++it) {
                if(it->first == str) {
                    break;
                }
            }
            
            return it;
        }
    
        std::size_t size() const {
            return values.size();
        }

        void erase(typename std::vector<std::pair<std::string, T>>::iterator iterator) {
            values.erase(iterator, values.end());
        }

        void erase(const std::string& str) {
            auto it = std::remove_if(values.begin(), values.end(), [&] (std::pair<std::string, T>& value) {
                return value.first == str;
            });
            if(it != values.end())
                values.erase(it, values.end());
        }
	};
}
