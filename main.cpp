//每日更新-2025年5月3日
#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>

using namespace std;

//===== 常量定义 =====
const int WIN_W = 1366;
const int WIN_H = 768;
const int SIDEBAR_W = 350;    // 调整侧边栏宽度
const int DEBUG_W = 400;      // 调整调试区宽度
const int WORK_AREA_W = WIN_W - SIDEBAR_W - DEBUG_W;
const int MAGNETIC_DIST = 10;
const COLORREF BLOCK_COLORS[] = {
    RGB(52, 152, 219),       // 蓝色
    RGB(46, 204, 113),        // 绿色
    RGB(231, 76, 60),        // 红色
    RGB(241, 196, 15),        // 黄色
    RGB(155, 89, 182),        // 紫色
    RGB(52, 152, 219)         // 蓝色
};

//===== 枚举类型定义 =====
enum BlockType {
    BLOCK_INCLUDE,
    BLOCK_USING_NAMESPACE,
    BLOCK_MAIN,
    BLOCK_RETURN,
    BLOCK_LOOP,
    BLOCK_CONDITION,
    BLOCK_COUT,
    BLOCK_CIN,
    BLOCK_MATH,
    BLOCK_LOGIC,
    BLOCK_COMMENT,
    BLOCK_FUNCTION,
    BLOCK_CLASS,
    BLOCK_ARRAY,
    BLOCK_TYPEDEF,
    BLOCK_WIDE_CHAR,
    BLOCK_MAX
};

//===== 代码块结构 =====
struct CodeBlock {
    BlockType type;
    string content;
    string internalText;
    int x, y;
    bool isTemplate;
    bool selected;
    bool isEditable;
    COLORREF textColor;

    CodeBlock(BlockType t = BLOCK_INCLUDE, const string& c = "", const string& it = "",
              int posX = 0, int posY = 0, bool temp = false, bool edit = false)
        : type(t), content(c), internalText(it), x(posX), y(posY),
          isTemplate(temp), selected(false), isEditable(edit), textColor(RGB(255, 255, 255)) {
    }

    // 重载等于运算符，用于比较 CodeBlock 对象
    bool operator==(const CodeBlock& other) const {
        return &other == this;
    }

    void Render(HDC hdc) const {
        COLORREF fillColor = isTemplate ? RGB(44, 62, 80) : BLOCK_COLORS[static_cast<int>(type) % BLOCK_MAX];
        COLORREF borderColor = selected ? RGB(236, 240, 241) : RGB(70, 70, 70);

        HBRUSH brush = CreateSolidBrush(fillColor);
        HPEN pen = CreatePen(PS_SOLID, 2, borderColor);
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);

        // 根据不同类型的代码块绘制不同的形状
        switch (type) {
            case BLOCK_MAIN:
                RoundRect(hdc, x, y, x + 240, y + 100, 10, 10);
                MoveToEx(hdc, x + 20, y + 50, NULL);
                LineTo(hdc, x + 220, y + 50);
                break;
            case BLOCK_LOOP:
                RoundRect(hdc, x, y, x + 200, y + 120, 15, 15);
                MoveToEx(hdc, x + 30, y + 20, NULL);
                LineTo(hdc, x + 170, y + 20);
                LineTo(hdc, x + 170, y + 100);
                LineTo(hdc, x + 30, y + 100);
                break;
            case BLOCK_CONDITION:
                Ellipse(hdc, x, y, x + 200, y + 100);
                break;
            case BLOCK_MATH:
                RoundRect(hdc, x, y, x + 280, y + 80, 10, 10);
                break;
            case BLOCK_FUNCTION:
                RoundRect(hdc, x, y, x + 260, y + 120, 10, 10);
                MoveToEx(hdc, x + 20, y + 60, NULL);
                LineTo(hdc, x + 240, y + 60);
                break;
            default:
                RoundRect(hdc, x, y, x + 240, y + 60, 10, 10);
                break;
        }

        // 设置文本颜色
        SetTextColor(hdc, textColor);
        SetBkMode(hdc, TRANSPARENT);
        
        // 绘制代码块内容
        RECT textRect;
        switch (type) {
            case BLOCK_MAIN:
            case BLOCK_FUNCTION:
                textRect = {x + 25, y + 20, x + 215, y + 80};
                DrawTextA(hdc, content.c_str(), -1, const_cast<LPRECT>(&textRect), DT_LEFT | DT_WORDBREAK);
                break;
            case BLOCK_MATH:
                textRect = {x + 30, y + 20, x + 250, y + 60};
                DrawTextA(hdc, content.c_str(), -1, const_cast<LPRECT>(&textRect), DT_LEFT | DT_WORDBREAK);
                break;
            default:
                textRect = {x + 15, y + 15, x + 225, y + 45};
                DrawTextA(hdc, content.c_str(), -1, const_cast<LPRECT>(&textRect), DT_LEFT);
                break;
        }

        // 如果被选中且不是模板，绘制删除按钮
        if (selected && !isTemplate) {
            HBRUSH delBrush = CreateSolidBrush(RGB(231, 76, 60));
            SelectObject(hdc, delBrush);
            Ellipse(hdc, x + 220, y + 5, x + 235, y + 20);
            DeleteObject(delBrush);
        }

        DeleteObject(brush);
        DeleteObject(pen);
    }

    bool HitTest(int mx, int my) const {
        switch (type) {
            case BLOCK_MAIN:
            case BLOCK_FUNCTION:
                return mx > x && mx < x + 240 && my > y && my < y + 100;
            case BLOCK_LOOP:
                return mx > x && mx < x + 200 && my > y && my < y + 120;
            case BLOCK_CONDITION:
                return mx > x && mx < x + 200 && my > y && my < y + 100;
            case BLOCK_MATH:
                return mx > x && mx < x + 280 && my > y && my < y + 80;
            default:
                return mx > x && mx < x + 240 && my > y && my < y + 60;
        }
    }
};

// 重载等于运算符，用于比较 CodeBlock 指针和 CodeBlock 对象
bool operator==(const CodeBlock* const& a, const CodeBlock& b) {
    return a == &b;
}

// 重载等于运算符，用于比较 CodeBlock 对象和 CodeBlock 指针
bool operator==(const CodeBlock& a, const CodeBlock* const& b) {
    return &a == b;
}

//===== 全局变量 =====
vector<CodeBlock> blocks;
vector<CodeBlock> templates;
CodeBlock* draggedBlock = nullptr;
CodeBlock* selectedBlock = nullptr;
POINT dragOffset;
string debugCode;
int scrollPos = 0;
int templateScrollPos = 0; // 新增侧边栏滚动位置
bool capturingDrag = false;

// 语法高亮设置
map<BlockType, COLORREF> syntaxHighlighting = {
    {BLOCK_INCLUDE, RGB(158, 218, 229)},    // 预处理指令
    {BLOCK_USING_NAMESPACE, RGB(158, 218, 229)}, // 命名空间
    {BLOCK_MAIN, RGB(174, 199, 232)},       // 主函数
    {BLOCK_RETURN, RGB(174, 199, 232)},     // return 语句
    {BLOCK_LOOP, RGB(235, 84, 103)},        // 循环
    {BLOCK_CONDITION, RGB(198, 153, 223)},  // 条件语句
    {BLOCK_COUT, RGB(198, 153, 223)},       // 输出
    {BLOCK_CIN, RGB(198, 153, 223)},        // 输入
    {BLOCK_MATH, RGB(101, 180, 218)},       // 数学运算
    {BLOCK_LOGIC, RGB(101, 180, 218)},      // 逻辑运算
    {BLOCK_COMMENT, RGB(151, 160, 137)},    // 注释
    {BLOCK_FUNCTION, RGB(255, 223, 107)},   // 函数
    {BLOCK_CLASS, RGB(255, 128, 128)}       // 类
};

// 双缓冲类
class DoubleBuffer {
    HDC hdcMem;
    HBITMAP bmp;
    int width, height;
public:
    DoubleBuffer(HDC hdc, int w, int h) : width(w), height(h) {
        hdcMem = CreateCompatibleDC(hdc);
        bmp = CreateCompatibleBitmap(hdc, w, h);
        SelectObject(hdcMem, bmp);
    }

    ~DoubleBuffer() {
        DeleteDC(hdcMem);
        DeleteObject(bmp);
    }

    operator HDC() { return hdcMem; }

    void Blit(HDC hdc) {
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
    }
};

// 初始化模板
void InitTemplates() {
    templates.clear();
    templates.emplace_back(BLOCK_INCLUDE, "#include <iostream>", "", 20, 20, true, false);
    templates.emplace_back(BLOCK_USING_NAMESPACE, "using namespace std;", "", 20, 100, true, false);
    templates.emplace_back(BLOCK_MAIN, "int main() {\n    \n}", "", 20, 170, true, false);
    templates.emplace_back(BLOCK_RETURN, "return 值;", "", 20, 250, true, false);
    templates.emplace_back(BLOCK_LOOP, "while (条件) {\n    // 循环体\n}", "", 20, 300, true, false);
    templates.emplace_back(BLOCK_CONDITION, "if (条件) {\n    // 条件体\n}", "", 20, 400, true, false);
    templates.emplace_back(BLOCK_COUT, "cout << \"输出内容\";", "", 20, 480, true, false);
    templates.emplace_back(BLOCK_CIN, "cin >> 变量名;", "", 20, 550, true, false);
    templates.emplace_back(BLOCK_MATH, "结果 = 运算表达式;", "", 20, 630, true, false);
    templates.emplace_back(BLOCK_LOGIC, "逻辑结果 = 条件1 && 条件2;", "", 20, 700, true, false);
    templates.emplace_back(BLOCK_COMMENT, "// 这是注释", "", 20, 760, true, false);
    templates.emplace_back(BLOCK_FUNCTION, "返回类型 函数名(参数) {\n    // 函数体\n}", "", 20, 820, true, false);
    
    // 新增常用模板
    templates.emplace_back(BLOCK_CLASS, "class 类名 {\npublic:\n    // 成员函数\nprivate:\n    // 成员变量\n};", "", 20, 900, true, false);
    templates.emplace_back(BLOCK_ARRAY, "类型 数组名[大小];", "", 20, 980, true, false);
    templates.emplace_back(BLOCK_TYPEDEF, "typedef 类型 别名;", "", 20, 1020, true, false);
    templates.emplace_back(BLOCK_WIDE_CHAR, "wchar_t 宽字符变量;", "", 20, 1070, true, false);
}

// 生成代码
void GenerateCode() {
    stringstream ss;
    vector<CodeBlock> sortedBlocks = blocks;
    sort(sortedBlocks.begin(), sortedBlocks.end(), [](const CodeBlock& a, const CodeBlock& b) {
        return a.y < b.y || (a.y == b.y && a.x < b.x);
    });

    // 处理 #include 和 using namespace
    for (const CodeBlock& block : sortedBlocks) {
        if (block.type == BLOCK_INCLUDE) {
            ss << block.content << "\n";
        } else if (block.type == BLOCK_USING_NAMESPACE) {
            ss << block.content << "\n\n";
        }
    }

    ss << "// 用户代码\n\n";

    // 处理主函数
    bool hasMain = false;
    for (const CodeBlock& block : sortedBlocks) {
        if (block.type == BLOCK_MAIN) {
            ss << block.content << "\n\n";
            hasMain = true;
            break;
        }
    }

    if (!hasMain) ss << "// 请从模板区拖拽代码块开始构建你的程序\n";

    // 处理其他代码块
    ss << "// 主函数内部\n";
    for (const CodeBlock& block : sortedBlocks) {
        if (block.type == BLOCK_INCLUDE || block.type == BLOCK_USING_NAMESPACE || 
            block.type == BLOCK_MAIN) continue;

        if (block.type == BLOCK_RETURN) {
            ss << "    return " << block.content.substr(7) << ";\n";
        } else {
            ss << "    " << block.content << "\n";
        }
    }

    debugCode = ss.str();
}

// 磁吸对齐
void MagneticAlignment(int& newX, int& newY) {
    newX = max(SIDEBAR_W + 20, min(newX, SIDEBAR_W + WORK_AREA_W - 240));
    newY = max(20, newY);

    for (const CodeBlock& block : blocks) {
        if (&block == draggedBlock) continue;

        // 垂直对齐
        if (abs(newX - block.x) < MAGNETIC_DIST) newX = block.x;
        if (abs(newX - (block.x + 240)) < MAGNETIC_DIST) newX = block.x + 240 - 240;

        // 水平对齐
        if (abs(newY - block.y) < MAGNETIC_DIST) newY = block.y;
        if (abs(newY - (block.y + 60)) < MAGNETIC_DIST) newY = block.y + 60;
        if (abs(newY - (block.y + 100)) < MAGNETIC_DIST) newY = block.y + 100;
    }
}

// 检查删除按钮
bool CheckDeleteButton(int x, int y, CodeBlock* block) {
    return x > block->x + 220 && x < block->x + 235 &&
           y > block->y + 5 && y < block->y + 20;
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HFONT fontMain, fontCode, fontSidebar;
    static HWND hDebugScrollView, hTemplateScrollView;
    static SHELLEXECUTEINFO shExecInfo;

    switch (msg) {
        case WM_CREATE:
            // 创建字体
            fontMain = CreateFont(24, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
            fontCode = CreateFont(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
            fontSidebar = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");

            // 创建滚动条
            hDebugScrollView = CreateWindowA("SCROLLBAR", NULL,
                WS_CHILD | WS_VISIBLE | SBS_VERT,
                WIN_W - DEBUG_W - 17, 0, 17, WIN_H,
                hwnd, (HMENU)1000, NULL, NULL);

            hTemplateScrollView = CreateWindowA("SCROLLBAR", NULL,
                WS_CHILD | WS_VISIBLE | SBS_VERT,
                SIDEBAR_W - 17, 0, 17, WIN_H,
                hwnd, (HMENU)1001, NULL, NULL);

            // 初始化模板
            InitTemplates();
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            DoubleBuffer db(hdc, rc.right, rc.bottom);
            FillRect(db, &rc, CreateSolidBrush(RGB(37, 46, 56))); // 深色背景

            // 绘制侧边栏背景
            RECT sidebar = {0, 0, SIDEBAR_W, rc.bottom};
            FillRect(db, &sidebar, CreateSolidBrush(RGB(28, 36, 45))); // 稍深侧边栏
            SelectObject(db, fontSidebar);
            SetTextColor(db, RGB(215, 215, 215));
            
            // 绘制侧边栏标题
            RECT sidebarTitle = {20, 20, SIDEBAR_W - 40, 60};
            DrawTextA(db, "代码块模板库", -1, const_cast<LPRECT>(&sidebarTitle), DT_CENTER | DT_VCENTER);

            // 绘制模板分割线
            MoveToEx(db, 0, 60, NULL);
            LineTo(db, SIDEBAR_W, 60);

            // 绘制模板滚动区域
            SetScrollPos(hTemplateScrollView, SB_CTL, templateScrollPos, TRUE);
            for (CodeBlock& temp : templates) {
                if (temp.y + 60 > templateScrollPos && temp.y < templateScrollPos + rc.bottom) {
                    temp.Render(db);
                }
            }

            // 绘制工作区背景
            RECT workArea = {SIDEBAR_W, 0, SIDEBAR_W + WORK_AREA_W, WIN_H};
            FillRect(db, &workArea, CreateSolidBrush(RGB(30, 35, 42)));

            // 绘制网格线
            HPEN gridPen = CreatePen(PS_DOT, 1, RGB(60, 65, 75));
            SelectObject(db, gridPen);
            for (int y = 0; y < rc.bottom; y += 20) {
                MoveToEx(db, SIDEBAR_W, y, NULL);
                LineTo(db, SIDEBAR_W + WORK_AREA_W, y);
            }

            // 绘制代码块
            for (CodeBlock& block : blocks) {
                block.Render(db);
            }

            // 绘制调试区域背景
            RECT debugArea = {WIN_W - DEBUG_W, 0, WIN_W, WIN_H};
            FillRect(db, &debugArea, CreateSolidBrush(RGB(20, 25, 30))); // 稍深调试区
            SelectObject(db, fontCode);
            SetTextColor(db, RGB(225, 225, 225));

            // 计算调试区域代码高度
            int codeHeight = debugCode.size() * 30;
            RECT codeRect = {WIN_W - DEBUG_W + 20, -scrollPos, WIN_W - 40, WIN_H - scrollPos + 100};
            DrawTextA(db, debugCode.c_str(), -1, &codeRect, DT_TOP | DT_LEFT | DT_WORDBREAK);

            // 绘制复制按钮
            SetTextColor(db, RGB(150, 150, 150));
            RECT copyBtnRect = {WIN_W - DEBUG_W + 20, 20, WIN_W - 60, 50};
            DrawTextA(db, "点击复制代码", -1, &copyBtnRect, DT_CENTER | DT_VCENTER);

            // 更新滚动条范围
            SetScrollRange(hDebugScrollView, SB_CTL, 0, max(codeHeight - (WIN_H - 60), 0), TRUE);
            SetScrollPos(hDebugScrollView, SB_CTL, scrollPos, TRUE);

            DeleteObject(gridPen);
            db.Blit(hdc);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_VSCROLL:
            if ((HWND)lp == hDebugScrollView) {
                int action = wp;
                int pos = 0;

                switch (action) {
                    case SB_LINEDOWN: pos = scrollPos + 30; break;
                    case SB_LINEUP: pos = scrollPos - 30; break;
                    case SB_PAGEDOWN: pos = scrollPos + 150; break;
                    case SB_PAGEUP: pos = scrollPos - 150; break;
                    case SB_THUMBTRACK: pos = HIWORD(wp); break;
                    default: pos = scrollPos;
                }

                pos = max(0, min(pos, max((int)debugCode.size() * 30 - (WIN_H - 60), 0)));
                scrollPos = pos;
                SetScrollPos(hDebugScrollView, SB_CTL, pos, TRUE);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if ((HWND)lp == hTemplateScrollView) {
                int action = wp;
                int pos = 0;

                switch (action) {
                    case SB_LINEDOWN: pos = templateScrollPos + 30; break;
                    case SB_LINEUP: pos = templateScrollPos - 30; break;
                    case SB_PAGEDOWN: pos = templateScrollPos + 150; break;
                    case SB_PAGEUP: pos = templateScrollPos - 150; break;
                    case SB_THUMBTRACK: pos = HIWORD(wp); break;
                    default: pos = templateScrollPos;
                }

                pos = max(0, min(pos, max(1200 - (WIN_H - 60), 0)));
                templateScrollPos = pos;
                SetScrollPos(hTemplateScrollView, SB_CTL, pos, TRUE);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lp);
            int y = GET_Y_LPARAM(lp);
            draggedBlock = nullptr;
            selectedBlock = nullptr;
        
            // 取消所有块的选中状态
            for (auto& block : blocks) {
                block.selected = false;
            }

            if (x < SIDEBAR_W) { // 模板区点击
                for (CodeBlock& temp : templates) {
                    if (temp.HitTest(x, y - templateScrollPos)) {
                        CodeBlock newBlock = temp;
                        newBlock.isTemplate = false;

                        // 确保新代码块不会与其他代码块重叠
                        newBlock.x = SIDEBAR_W + 50;
                        newBlock.y = 100;

                        for (const CodeBlock& block : blocks) {
                            if (newBlock.x + 240 > block.x && newBlock.x < block.x + 240 &&
                                newBlock.y + 60 > block.y && newBlock.y < block.y + 60) {
                                newBlock.y += 70;
                            }
                        }

                        blocks.push_back(newBlock);
                        draggedBlock = &blocks.back();
                        dragOffset.x = x - (newBlock.x);
                        dragOffset.y = y - newBlock.y;
                        SetCapture(hwnd);
                        break;
                    }
                }
            } 
            else if (x >= SIDEBAR_W && x < SIDEBAR_W + WORK_AREA_W) { // 工作区点击
                for (auto& block : blocks) {
                    if (block.HitTest(x, y)) {
                        draggedBlock = &block;
                        dragOffset.x = x - block.x;
                        dragOffset.y = y - block.y;
            
                        selectedBlock = &block;
                        selectedBlock->selected = true;
            
                        if (CheckDeleteButton(x, y, selectedBlock)) {
                            for (auto it = blocks.begin(); it != blocks.end(); ++it) {
                                if (&(*it) == selectedBlock) {
                                    blocks.erase(it);
                                    GenerateCode();
                                    InvalidateRect(hwnd, NULL, TRUE);
                                    return 0;
                                }
                            }
                        }
                        break;
                    }
                }
            }
            else if (x >= WIN_W - DEBUG_W && x <= WIN_W - DEBUG_W + 200 && y >= 20 && y <= 50) {
                // 复制代码功能
                OpenClipboard(hwnd);
                EmptyClipboard();
                HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, debugCode.size() + 1);
                memcpy(GlobalLock(hg), debugCode.c_str(), debugCode.size() + 1);
                GlobalUnlock(hg);
                SetClipboardData(CF_TEXT, hg);
                CloseClipboard();
            }
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case WM_MOUSEMOVE:
            if (draggedBlock && (wp & MK_LBUTTON)) {
                int newX = GET_X_LPARAM(lp) - dragOffset.x;
                int newY = GET_Y_LPARAM(lp) - dragOffset.y;
        
                MagneticAlignment(newX, newY);
        
                // 检查是否与其他代码块重叠
                bool overlaps = false;
                for (const CodeBlock& block : blocks) {
                    if (&block == draggedBlock) continue;
                    if (newX + 240 > block.x && newX < block.x + 240 &&
                        newY + 60 > block.y && newY < block.y + 60) {
                        overlaps = true;
                        break;
                    }
                }
        
                if (overlaps) {
                    // 如果重叠，恢复到上一次的位置
                    newX = draggedBlock->x;
                    newY = draggedBlock->y;
                }
        
                if (newX != draggedBlock->x || newY != draggedBlock->y) {
                    RECT oldRect = {
                        draggedBlock->x - 5, draggedBlock->y - 5,
                        draggedBlock->x + 240, draggedBlock->y + ((draggedBlock->type == BLOCK_MAIN) ? 105 : 60)
                    };
                    InvalidateRect(hwnd, &oldRect, FALSE);
        
                    draggedBlock->x = newX;
                    draggedBlock->y = newY;
        
                    RECT newRect = {
                        newX - 5, newY - 5,
                        newX + 240, newY + ((draggedBlock->type == BLOCK_MAIN) ? 105 : 60)
                    };
                    InvalidateRect(hwnd, &newRect, FALSE);
        
                    GenerateCode();
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            break;

        case WM_KEYDOWN:
            if (wp == VK_DELETE && selectedBlock) {
                blocks.erase(find(blocks.begin(), blocks.end(), selectedBlock));
                GenerateCode();
                InvalidateRect(hwnd, NULL, TRUE);
            } else if (wp == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                // Ctrl+S 保存文件功能
                memset(&shExecInfo, 0, sizeof(shExecInfo));
                shExecInfo.cbSize = sizeof(shExecInfo);
                shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                shExecInfo.hwnd = hwnd;
                shExecInfo.lpVerb = "save";
                shExecInfo.lpFile = "generated_code.cpp";
                shExecInfo.nShow = SW_NORMAL;
                ShellExecuteEx(&shExecInfo);
            }
            break;
        case WM_LBUTTONUP:
            if (draggedBlock) {
                if (draggedBlock->x < SIDEBAR_W) {
                    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
                        if (&(*it) == draggedBlock) {
                            blocks.erase(it);
                            GenerateCode();
                            InvalidateRect(hwnd, NULL, TRUE);
                            break;
                        }
                    }
                }
                draggedBlock = nullptr;
                ReleaseCapture();
            }
            break;
        case WM_SIZE:
            if (hDebugScrollView) {
                MoveWindow(hDebugScrollView, WIN_W - DEBUG_W - 17, 0, 17, WIN_H, TRUE);
            }
            if (hTemplateScrollView) {
                MoveWindow(hTemplateScrollView, SIDEBAR_W - 17, 0, 17, WIN_H, TRUE);
            }
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            DeleteObject(fontMain);
            DeleteObject(fontCode);
            DeleteObject(fontSidebar);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

// 程序入口
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "CodeBlockEditor";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 31, 35));
    RegisterClass(&wc);

    HWND hwnd = CreateWindowA("CodeBlockEditor", "可视化代码编辑器",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            WIN_W, WIN_H, NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
