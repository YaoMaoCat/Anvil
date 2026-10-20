#pragma once

#include <string>

namespace anvil::ui {

// <eclass type=.. value=.. teacher=..> 的三个字段
// 1=站点(1 教师+N 学生) 2=单席位(教师) 4=单席位(学生) 8=学校(N 教师+N 学生)
// 依据：DLL 里只有 type==8 才会解析 teacher 属性；1/2 已实测导入。
struct LicenseType {
    int type = 1;       // 1=站点 2=单席位(教师) 4=单席位(学生) 8=学校
    int teachers = 1;
    int students = 1;
};

// 画「授权类型 / 教师授权 / 学生授权」三行；返回 true 表示本帧有改动
bool DrawLicenseTypeFields(LicenseType* value, float scale);

// 类型名，用于状态栏提示
std::string LicenseTypeName(int type);

// 该类型是否需要教师数（单席位教师版没有学生数，反之亦然）
bool LicenseTypeUsesTeachers(int type);

} // namespace anvil::ui
