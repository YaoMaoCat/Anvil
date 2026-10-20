#include "ui/license_type.h"

#include <imgui.h>

namespace anvil::ui {
namespace {

// 取值依据 2016 版 LibLicenseUI.dll 的 eclass 解析段（0x1000f5a0 起）：
//   cmp [type], 8 / jne → 只有 type==8 才会去读 teacher 属性，
// 说明 8 是「多教师」的学校授权，1/2/4 都是单教师形态。
// 再结合语言包四条文案（PERSITE / PERSCHOOL / PERSEAT_TEACHER / PERSEAT_STUDENT）得到下表。
struct TypeOption {
    int value;
    const char* label;
    const char* note;
    bool multi_teacher;
    bool multi_student;
};

constexpr TypeOption kOptions[] = {
    {1, "站点授权", "1 教师 + N 学生", false, true},
    {2, "单席位（教师）", "1 教师", false, false},
    {4, "单席位（学生）", "1 学生", false, false},
    {8, "学校授权", "N 教师 + N 学生", true, true},
};

const TypeOption& Option(int type) {
    for (const TypeOption& option : kOptions) {
        if (option.value == type) {
            return option;
        }
    }
    return kOptions[0];
}

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
    return Option(type).label;
}

bool LicenseTypeUsesTeachers(int type) {
    return Option(type).multi_teacher;
}

bool DrawLicenseTypeFields(LicenseType* value, float scale) {
    bool changed = false;
    const float label_x = 84.0f * scale;
    const float width = 196.0f * scale;
    const TypeOption& current = Option(value->type);

    ImGui::TextDisabled("授权类型");
    ImGui::SameLine(label_x);
    ImGui::SetNextItemWidth(width);
    const int index = IndexOf(value->type);
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

    // 只有学校授权用得上教师数；单席位连学生数也固定 1
    ImGui::BeginDisabled(!current.multi_teacher);
    ImGui::TextDisabled("教师授权");
    ImGui::SameLine(label_x);
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputInt("##teachers", &value->teachers, 0, 0)) {
        value->teachers = value->teachers < 1 ? 1 : value->teachers;
        changed = true;
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!current.multi_student);
    ImGui::TextDisabled("学生授权");
    ImGui::SameLine(label_x);
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputInt("##students", &value->students, 0, 0)) {
        value->students = value->students < 1 ? 1 : value->students;
        changed = true;
    }
    ImGui::EndDisabled();

    if (current.value == 4 || current.value == 8) {
        ImGui::TextColored(ImVec4(0.75f, 0.35f, 0.05f, 1.0f),
                           "该类型需要教师端掩码支持（2017 版实测掩码=3，只认 1/2）；导入返回 0x65 即不支持");
    }
    return changed;
}

} // namespace anvil::ui
