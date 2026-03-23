#ifndef __MENU_LIST_H
#define __MENU_LIST_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>
#include "app_data.h"

/* ── 配置参数 ────────────────── */
// 移除 MENU_MAX_NODES，改为动态分配
#define MENU_GAP          56     // 图标之间的间距
#define MENU_CX           120    // 屏幕水平中心
#define MENU_ICON_Y       160    // 图标垂直中心位置
#define MENU_ICON_W       52     // 图标宽度
#define MENU_ICON_H       52     // 图标高度

/* ── 节点结构体 ────────────────────────────────── */
typedef struct MenuItem {
    char             name[16];    // 显示名称
    char             icon[8];     // 图标（LV_SYMBOL_xxx）
    struct MenuItem *next;        // 链表：同级下一项
    struct MenuItem *sub;         // 预留：子菜单入口
    struct MenuItem *back;        // 返回：指向父级菜单节点
    void           (*action)(void); // 点击回调（跳转App页面）
    
    /* UI 相关指针 */
    lv_obj_t        *lv_obj;      // 按钮对象
    lv_obj_t        *name_obj;    // 名称 Label 对象
    int32_t          x;           // 相对屏幕中心的实时 x 偏移
} MenuItem_t;

/* ── 核心生命周期管理 ────────────────────────── */

/**
 * @brief 初始化菜单并分配动态内存
 */
void Menu_Init(void);

/**
 * @brief 彻底销毁菜单链表并释放 malloc 的内存
 * @note  在进入具体 App 页面（如 Music）前必须调用
 */
void Menu_Deinit(void);

/**
 * @brief 动态创建一个菜单节点
 */
MenuItem_t *menu_create_item(const char *name, 
                             const char *icon, 
                             void (*action)(void), 
                             MenuItem_t *back);

/* ── UI 渲染与导航 ────────────────────────────── */

/**
 * @brief 渲染指定的菜单层级
 */
void menu_render(MenuItem_t *root);

/**
 * @brief 返回逻辑：释放当前并重新构建上级菜单
 */
void menu_go_back(void);

/* ── 状态栏组件 ──────────────────────────────── */

/**
 * @brief 更新状态栏数据（温湿度、时间、GPS等）
 */
void menu_update_statusbar(StatusBar_Data_t *data);

/**
 * @brief 强制隐藏状态栏
 */
void menu_clear_statusbar(void);

#endif