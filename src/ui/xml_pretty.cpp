#include "ui/xml_pretty.h"

namespace anvil::ui::xml_pretty {

std::string Format(const std::string& xml) {
    std::string out;
    out.reserve(xml.size() * 2);

    int depth = 0;
    bool last_was_text = false;
    std::size_t i = 0;

    while (i < xml.size()) {
        if (xml[i] == '<') {
            const std::size_t end = xml.find('>', i);
            if (end == std::string::npos) {
                out.append(xml, i, std::string::npos);
                break;
            }
            const std::string tag = xml.substr(i, end - i + 1);
            const bool closing = tag.size() > 1 && tag[1] == '/';
            const bool self_closing = tag.size() > 2 && tag[tag.size() - 2] == '/';
            const bool declaration = tag.size() > 1 && (tag[1] == '?' || tag[1] == '!');

            if (closing && depth > 0) {
                --depth;
            }
            // 文本节点之后紧跟闭合标签时保持同行，例如 <serialnumber>XXXX</serialnumber>
            if (!out.empty() && !last_was_text) {
                out.push_back('\n');
                out.append(static_cast<std::size_t>(depth) * 2, ' ');
            }
            out += tag;
            if (!closing && !self_closing && !declaration) {
                ++depth;
            }
            last_was_text = false;
            i = end + 1;
            continue;
        }

        const std::size_t next = xml.find('<', i);
        const std::size_t count = (next == std::string::npos ? xml.size() : next) - i;
        out.append(xml, i, count);
        last_was_text = count > 0;
        i += count;
    }
    return out;
}

} // namespace anvil::ui::xml_pretty
