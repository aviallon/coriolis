#ifndef __LIB_WIRE_LOAD_H__
#define __LIB_WIRE_LOAD_H__

#include "vlsisapd/liberty/Attribute.h"

namespace LIB {

class WireLoad {
    public:
    WireLoad(Name name);
    
    inline Name getName();
    inline std::map<Name, Attribute*>& getAttributes();
    Attribute* getAttribute(Name attrName);

    void addAttribute(Name attrName, Attribute::Type attrType, const std::string& attrValue, const std::string& attrValue2 = "");

    const std::string getString()const;
    void print();
    bool write(std::ofstream &file);

    private:
    Name _name;
    std::map<Name, Attribute*> _attributes;
};
    
inline Name                        WireLoad::getName()       { return _name; };
inline std::map<Name, Attribute*>& WireLoad::getAttributes() { return _attributes; };
    
} // namespace LIB
#endif
