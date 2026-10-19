#pragma once

#include <string>

namespace anvil::ui {

// <eclass type=.. value=.. teacher=..> 的三个字段
// 类型取值来自客户端资源里的三类文案（PERSITE / PERSCHOOL / PERSEAT），
// 其中 1、2 已在本机 2016/2017 版实测通过；4、8 是单席位，教师端掩码=3 会拒绝（返回 0x65）。
struct LicenseType {
    int type = 1;       // 1=站点 2=学校 4=单席位(学生) 8=单席位(教师)
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
