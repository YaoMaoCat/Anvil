#include "ui/license_type.h"

#include <imgui.h>

namespace anvil::ui {
namespace {

struct TypeOption {
    int value;
    const char* label;
    const char* note;
};

// 文案对应语言包里的 PERSITE / PERSCHOOL / PERSEAT_*
constexpr TypeOption kOptions[] = {
    {1, "站点授权", "1 教师 + N 学生"},
    {2, "学校授权", "N 教师 + N 学生"},
    {4, "单席位（学生）", "1 学生"},
    {8, "单席位（教师）", "1 教师"},
};

int IndexOf(int type) {
    for (int i = 0; i < static_cast<int>(sizeof(kOptions) / sizeof(kOptions[0])); ++i) {
        if (kOptions[i].value == type) {
            return i;
        }
    }
    return 0;
}

} // namespace

std::string LicenseTypeName(int type) {
    return kOptions[IndexOf(type)].label;
}

bool LicenseTypeUsesTeachers(int type) {
    return type != 4;
}

bool DrawLicenseTypeFields(LicenseType* value, float scale) {
    bool changed = false;
    const float label_x = 84.0f * scale;
    const float width = 196.0f * scale;

    ImGui::TextDisabled("授权类型");
    ImGui::SameLine(label_x);
    ImGui::SetNextItemWidth(width);
    int index = IndexOf(value->type);
    if (ImGui::BeginCombo("##etype", kOptions[index].label)) {
        for (int i = 0; i < static_cast<int>(sizeof(kOptions) / sizeof(kOptions[0])); ++i) {
            const bool selected = i == index;
            if (ImGui::Selectable(kOptions[i].label, selected)) {
                value->type = kOptions[i].value;
                changed = true;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", kOptions[i].note);
        }
        ImGui::EndCombo();
    }

    // 单席位固定 1，不需要让用户改
    ImGui::BeginDisabled(!LicenseTypeUsesTeachers(value->type));
    ImGui::TextDisabled("教师授权");
    ImGui::SameLine(label_x);
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputInt("##teachers", &value->teachers, 0, 0)) {
        value->teachers = value->teachers < 1 ? 1 : value->teachers;
        changed = true;
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(value->type == 8);
    ImGui::TextDisabled("学生授权");
    ImGui::SameLine(label_x);
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputInt("##students", &value->students, 0, 0)) {
        value->students = value->students < 1 ? 1 : value->students;
        changed = true;
    }
    ImGui::EndDisabled();

    if (value->type != 1 && value->type != 2) {
        ImGui::TextColored(ImVec4(0.75f, 0.35f, 0.05f, 1.0f),
                           "单席位类型需要教师端掩码支持，本机实测只接受站点/学校（掩码=3）");
    }
    return changed;
}

} // namespace anvil::ui
