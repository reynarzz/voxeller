#pragma once
#include <unordered_map>


namespace Unvoxeller
{
    class FactoryBase
    {

    };

    template<typename K, typename V>
    class FactoryBase_T : public FactoryBase
    {
    public:
        virtual V Get(const K& key)
        {
            auto it = Elements.find(key);

            if(it != Elements.end())
            {
                return it->second;
            }

            return {};
        }
      //  FactoryBase operator=(const FactoryBase&) = delete;

    protected:
        std::unordered_map<K, V> Elements;
    };

}
