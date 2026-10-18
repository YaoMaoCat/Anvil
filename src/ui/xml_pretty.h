#pragma once

#include <string>

namespace anvil::ui::xml_pretty {

// 把单行 XML 拆成缩进形式，仅用于显示；文件内容不受影响
std::string Format(const std::string& xml);

} // namespace anvil::ui::xml_pretty
