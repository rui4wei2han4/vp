#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>

using namespace std;

//======= 双缓冲绘图类 ========
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

//======= 全局常量 ========
const int WIN_W = 1366, WIN_H = 768;
const int SIDEBAR_W = 240;
const int DEBUG_W = 360;
const int MAGNETIC_DIST = 15;
const COLORREF COLORS[] = {
    RGB(70, 130, 180),   // 函数-蓝
    RGB(60, 170, 90),    // 变量-绿
    RGB(220, 80, 60),    // 输出-红
    RGB(255, 193, 7)     // 模板-金
};

enum BlockType {
    BLOCK_FUNCTION = 0,
    BLOCK_VARIABLE,
    BLOCK_OUTPUT,
    BLOCK_TEMPLATE,
    BLOCK_INCLUDE,
    BLOCK_USING_NAMESPACE,
    BLOCK_MAIN,
    BLOCK_COUT,
    BLOCK_RETURN,
    BLOCK_LOOP,
    BLOCK_CONDITION,
    BLOCK_COMMENT,
    BLOCK_MATH,
    BLOCK_LOGIC,
    BLOCK_IO,
    BLOCK_MEMORY,
    BLOCK_MAX // 最大类型，用于数组边界
};

//======= 代码块结构 ========
struct CodeBlock {
    int type;          // 代码块类型
    string content;    // 内容
    string internalText; // 内部可编辑文本
    int x, y;          // 位置
    bool isTemplate;   // 是否为模板
    bool selected;     // 是否被选中
    bool isLoop;       // 是否为循环块
    bool isEditable;   // 是否可编辑
    RECT editRect;     // 编辑区域

    CodeBlock() : type(BLOCK_FUNCTION), x(0), y(0), isTemplate(false), selected(false), isLoop(false), isEditable(false) {}

    CodeBlock(int t, const string& c, const string& it, int posX, int posY, bool temp, bool edit)
        : type(t), content(c), internalText(it), x(posX), y(posY), isTemplate(temp), selected(false), isLoop(false), isEditable(edit) {
        if(isEditable && !isTemplate) {
            editRect = {x+30, y+30, x+210, y+50};
        }
    }

    void Render(HDC hdc) const {
        // 颜色选择
        COLORREF fillColor = isTemplate ? COLORS[BLOCK_TEMPLATE] : COLORS[type % BLOCK_MAX];
        COLORREF borderColor = selected ? RGB(255,255,255) : RGB(200,200,200);

        // 绘制主体
        HBRUSH brush = CreateSolidBrush(fillColor);
        HPEN pen = CreatePen(PS_SOLID, 2, borderColor);
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);

        switch(type) {
            case BLOCK_MAIN: // int main()
                Rectangle(hdc, x, y, x+240, y+100);
                MoveToEx(hdc, x, y+50, NULL);
                LineTo(hdc, x+240, y+50);
                break;
            case BLOCK_LOOP: // 循环块
                RoundRect(hdc, x, y, x+200, y+120, 15, 15);
                MoveToEx(hdc, x+50, y, NULL);
                LineTo(hdc, x+150, y);
                LineTo(hdc, x+150, y+120);
                LineTo(hdc, x+50, y+120);
                break;
            case BLOCK_CONDITION: // 条件块
                Ellipse(hdc, x, y, x+200, y+100);
                break;
            default:
                RoundRect(hdc, x, y, x+240, y+60, 10, 10);
                break;
        }

        // 文字内容
        SetTextColor(hdc, RGB(255,255,255));
        SetBkMode(hdc, TRANSPARENT);
        RECT textRC = {x+15, y+10, x + 225, y + 50};
        DrawTextA(hdc, content.c_str(), -1, const_cast<LPRECT>(&textRC), DT_WORDBREAK);

        // 内部编辑区域
        if(isEditable && !isTemplate) {
            FillRect(hdc, &editRect, CreateSolidBrush(RGB(40,40,40)));
            SetTextColor(hdc, RGB(200,200,255));
            DrawTextA(hdc, internalText.c_str(), -1, const_cast<LPRECT>(&editRect), DT_LEFT|DT_VCENTER);
        }

        // 删除按钮
        if(selected && !isTemplate) {
            HBRUSH delBrush = CreateSolidBrush(RGB(220, 80, 60));
            SelectObject(hdc, delBrush);
            Ellipse(hdc, x+220, y+5, x+235, y+20);
            DeleteObject(delBrush);
        }

        DeleteObject(brush);
        DeleteObject(pen);
    }

    bool HitTest(int mx, int my) const {
        switch(type) {
            case BLOCK_MAIN: return mx > x && mx < x+240 && my > y && my < y+100;
            case BLOCK_LOOP: return mx > x && mx < x+200 && my > y && my < y+120;
            case BLOCK_CONDITION: return mx > x && mx < x+200 && my > y && my < y+100;
            default: return mx > x && mx < x+240 && my > y && my < y+60;
        }
    }

    bool EditHitTest(int mx, int my) const {
        return mx >= editRect.left && mx <= editRect.right && 
               my >= editRect.top && my <= editRect.bottom;
    }
};

//======= 全局状态 ========
vector<CodeBlock> blocks;
vector<CodeBlock> templates;
CodeBlock* draggedBlock = NULL;
CodeBlock* selectedBlock = NULL;
CodeBlock* editedBlock = NULL;
POINT dragOffset;
string debugCode;
string userOutput = "这里是用户输入的文本";
int returnValue = 0;
bool isTyping = false;

//======= 功能函数 ========
void InitTemplates() {
    templates.clear();

    // 核心结构模板
    templates.emplace_back(BLOCK_INCLUDE, "#include<bits/stdc++.h>", "", 30, 30, true, false);
    templates.emplace_back(BLOCK_USING_NAMESPACE, "using namespace std;", "", 30, 120, true, false);
    templates.emplace_back(BLOCK_MAIN, "int main() {\n    // 代码块\n}", "", 30, 210, true, false);
    templates.emplace_back(BLOCK_RETURN, "return 0;", "", 30, 300, true, false);

    // 控制流模板
    templates.emplace_back(BLOCK_LOOP, "while (条件) {\n    // 循环体\n}", "", 30, 390, true, false);
    templates.emplace_back(BLOCK_CONDITION, "if (条件) {\n    // 条件体\n}", "", 30, 510, true, false);

    // 输入输出模板
    templates.emplace_back(BLOCK_COUT, "cout << \"输出内容\";", "", 30, 630, true, true);
    templates.emplace_back(BLOCK_IO, "cin >> 变量名;", "", 30, 720, true, true);

    // 数据操作模板
    templates.emplace_back(BLOCK_MATH, "结果 = 数学表达式;", "", 30, 810, true, true);
    templates.emplace_back(BLOCK_LOGIC, "逻辑运算结果 = 条件1 && 条件2;", "", 30, 880, true, true);

    // 注释模板
    templates.emplace_back(BLOCK_COMMENT, "// 这是注释", "", 30, 970, true, true);

    // 初始化可编辑区域
    for(auto& block : templates) {
        if(block.isEditable) {
            block.editRect = {block.x+30, block.y+30, block.x+210, block.y+50};
        }
    }
}

void GenerateCode() {
    stringstream ss;
    ss << "// 自动生成代码\n\n";
    bool hasInclude = false;
    bool hasUsingNamespace = false;
    bool hasMainStart = false;
    bool hasReturn = false;

    vector<CodeBlock> sortedBlocks = blocks;
    sort(sortedBlocks.begin(), sortedBlocks.end(), [](const CodeBlock& a, const CodeBlock& b) {
        if(a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });

    for(const CodeBlock& block : sortedBlocks) {
        if(block.isTemplate) continue;

        switch(block.type) {
            case BLOCK_INCLUDE:
                if (!hasInclude) {
                    ss << block.content << "\n";
                    hasInclude = true;
                }
                break;
            case BLOCK_USING_NAMESPACE:
                if (!hasUsingNamespace) {
                    ss << block.content << "\n";
                    hasUsingNamespace = true;
                }
                break;
            case BLOCK_MAIN:
                if (!hasMainStart) {
                    ss << "\n" << block.content << "\n";
                    hasMainStart = true;
                }
                break;
            case BLOCK_COUT:
                if (hasMainStart && !hasReturn) {
                    ss << "    " << block.content.substr(0, 14) << "\"" << block.internalText << "\";" << "\n";
                }
                break;
            case BLOCK_RETURN:
                if (hasMainStart && !hasReturn) {
                    ss << "    " << block.content.substr(0, 7) << block.internalText << ";" << "\n";
                    hasReturn = true;
                }
                break;
            case BLOCK_LOOP:
                if (hasMainStart && !hasReturn) {
                    string loopContent = block.content;
                    size_t startPos = loopContent.find('{') + 1;
                    size_t endPos = loopContent.find('}');
                    if(startPos != string::npos && endPos != string::npos) {
                        loopContent = loopContent.substr(0, startPos) + block.internalText + loopContent.substr(endPos);
                    }
                    ss << "    " << loopContent << "\n";
                }
                break;
            case BLOCK_CONDITION:
                if (hasMainStart && !hasReturn) {
                    string condContent = block.content;
                    size_t startPos = condContent.find('{') + 1;
                    size_t endPos = condContent.find('}');
                    if(startPos != string::npos && endPos != string::npos) {
                        condContent = condContent.substr(0, startPos) + block.internalText + condContent.substr(endPos);
                    }
                    ss << "    " << condContent << "\n";
                }
                break;
            default:
                if (hasMainStart && !hasReturn) {
                    ss << "    " << block.content << "\n";
                }
                break;
        }
    }

    if (hasMainStart && !hasReturn) {
        ss << "    return 0;\n";
    }
    if (hasMainStart) {
        ss << "}\n";
    }
    debugCode = ss.str();
}

void MagneticAlignment(int& newX, int& newY) {
    // 边界磁吸
    if(abs(newX - SIDEBAR_W) < MAGNETIC_DIST) newX = SIDEBAR_W;
    if(abs(newX + 240 - (WIN_W-DEBUG_W)) < MAGNETIC_DIST) 
        newX = WIN_W - DEBUG_W - 240;
    if(abs(newY - 20) < MAGNETIC_DIST) newY = 20;

    // 块间磁吸
    for(const CodeBlock& block : blocks) {
        if(&block == draggedBlock) continue;

        if(abs(newX - block.x) < MAGNETIC_DIST) newX = block.x;
        if(abs(newY - block.y) < MAGNETIC_DIST) newY = block.y;
        if(abs((newX+240) - (block.x + (block.isTemplate ? 200 : 240))) < MAGNETIC_DIST)
            newX = block.x + (block.isTemplate ? 200 : 240) - 240;
        if(abs((newY+60) - (block.y + (block.isTemplate ? 80 : ((block.type == BLOCK_MAIN) ? 100 : 60)))) < MAGNETIC_DIST)
            newY = block.y + (block.isTemplate ? 80 : ((block.type == BLOCK_MAIN) ? 100 : 60)) - ((draggedBlock->type == BLOCK_MAIN) ? 100 : 60);
    }
}

bool CheckDeleteButton(int mx, int my) {
    if(!selectedBlock) return false;
    return mx > selectedBlock->x+220 && mx < selectedBlock->x+235 &&
           my > selectedBlock->y+5 && my < selectedBlock->y+20;
}

//======= 窗口过程 ========
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HFONT fontMain, fontCode;

    switch(msg) {
    case WM_CREATE:
        fontMain = CreateFont(18,0,0,0,FW_SEMIBOLD,0,0,0,ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY,VARIABLE_PITCH,"Segoe UI");
        fontCode = CreateFont(16,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY,FIXED_PITCH,"Consolas");

        InitTemplates();
        GenerateCode();
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        
        DoubleBuffer db(hdc, rc.right, rc.bottom);
        FillRect(db, &rc, CreateSolidBrush(RGB(36, 42, 49)));
        
        // 侧边栏
        RECT sidebar = {0,0,SIDEBAR_W,rc.bottom};
        FillRect(db, &sidebar, CreateSolidBrush(RGB(47, 54, 62)));
        
        // 绘制字体
        SelectObject(db, fontMain);

        // 绘制模板标题
        SetTextColor(db, RGB(200,200,200));
        DrawTextA(db, "模板库", -1, const_cast<LPRECT>(&sidebar), DT_LEFT|DT_VCENTER);

        // 绘制模板
        for(const CodeBlock& temp : templates) {
            temp.Render(db);
        }
        
        // 绘制代码块
        for(const CodeBlock& block : blocks) {
            block.Render(db);
        }
        
        // 调试区
        SelectObject(db, fontCode);
        RECT debugRC = {WIN_W-DEBUG_W,0,WIN_W,WIN_H};
        FillRect(db, &debugRC, CreateSolidBrush(RGB(28, 35, 41)));
        SetTextColor(db, RGB(214, 225, 235));
        DrawTextA(db, debugCode.c_str(), -1, &debugRC, DT_TOP|DT_LEFT|DT_WORDBREAK);
        
        db.Blit(hdc);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lp);
        int y = GET_Y_LPARAM(lp);
        draggedBlock = NULL;
        selectedBlock = NULL;
        isTyping = false;

        if(x < SIDEBAR_W) { // 模板区点击
            for(CodeBlock& temp : templates) {
                if(temp.HitTest(x, y)) {
                    CodeBlock newBlock = temp;
                    newBlock.isTemplate = false;
                    newBlock.x = SIDEBAR_W + 50;
                    newBlock.y = 100;
                    blocks.push_back(newBlock);
                    draggedBlock = &blocks.back();
                    dragOffset.x = x - draggedBlock->x;
                    dragOffset.y = y - draggedBlock->y;
                    SetCapture(hwnd);
                    break;
                }
            }
        } 
        else { // 工作区点击
            for(CodeBlock& block : blocks) {
                if(block.HitTest(x, y)) {
                    draggedBlock = &block;
                    dragOffset.x = x - block.x;
                    dragOffset.y = y - block.y;
                    
                    selectedBlock = &block;
                    selectedBlock->selected = true;
                    if(CheckDeleteButton(x, y)) { // 删除按钮点击
                        for(auto it = blocks.begin(); it != blocks.end(); ++it) {
                            if(&(*it) == selectedBlock) {
                                blocks.erase(it);
                                GenerateCode();
                                RECT updateRC = {WIN_W-DEBUG_W,0,WIN_W,WIN_H};
                                InvalidateRect(hwnd, &updateRC, FALSE);
                                return 0;
                            }
                        }
                    } else if(block.isEditable && block.EditHitTest(x, y)) {
                        isTyping = true;
                        editedBlock = &block;
                    }
                    break;
                }
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    case WM_MOUSEMOVE:
        if(draggedBlock && (wp & MK_LBUTTON)) {
            int newX = GET_X_LPARAM(lp) - dragOffset.x;
            int newY = GET_Y_LPARAM(lp) - dragOffset.y;

            // 范围约束
            newX = max(SIDEBAR_W+20, min(newX, WIN_W-DEBUG_W-240));
            newY = max(20, newY);
            
            // 磁吸对齐
            MagneticAlignment(newX, newY);
            
            // 更新位置并重绘
            if(newX != draggedBlock->x || newY != draggedBlock->y) {
                RECT oldArea = {
                    draggedBlock->x-5, draggedBlock->y-5,
                    draggedBlock->x + (draggedBlock->isTemplate ? 205 : ((draggedBlock->type == BLOCK_MAIN) ? 245 : 240)),
                    draggedBlock->y + (draggedBlock->isTemplate ? ((draggedBlock->type == BLOCK_LOOP) ? 125 : 85) : ((draggedBlock->type == BLOCK_MAIN) ? 105 : 65))
                };
                InvalidateRect(hwnd, &oldArea, FALSE);
                
                draggedBlock->x = newX;
                draggedBlock->y = newY;
                
                RECT newArea = {
                    newX-5, newY-5,
                    newX + (draggedBlock->isTemplate ? 205 : ((draggedBlock->type == BLOCK_MAIN) ? 245 : 240)),
                    newY + (draggedBlock->isTemplate ? ((draggedBlock->type == BLOCK_LOOP) ? 125 : 85) : ((draggedBlock->type == BLOCK_MAIN) ? 105 : 65))
                };
                InvalidateRect(hwnd, &newArea, FALSE);
                
                GenerateCode();
                RECT debugArea = {WIN_W-DEBUG_W,0,WIN_W,WIN_H};
                InvalidateRect(hwnd, &debugArea, TRUE);
            }
        }
        break;

    case WM_LBUTTONUP:
        if(draggedBlock) {
            draggedBlock = NULL;
            ReleaseCapture();
        }
        break;

    case WM_KEYDOWN:
        if(editedBlock && isTyping) {
            if(wp == VK_BACK) {
                if(!editedBlock->internalText.empty()) {
                    editedBlock->internalText.pop_back();
                    GenerateCode();
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            } else if(wp == VK_RETURN) {
                isTyping = false;
                editedBlock = NULL;
            } else if(wp >= 32 && wp <= 126) {
                editedBlock->internalText += (char)wp;
                GenerateCode();
                InvalidateRect(hwnd, NULL, TRUE);
            }
        } else if(wp == VK_DELETE && selectedBlock) {
            for(auto it = blocks.begin(); it != blocks.end(); ++it) {
                if(&(*it) == selectedBlock) {
                    blocks.erase(it);
                    GenerateCode();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                }
            }
        }
        break;

    case WM_CHAR:
        if(editedBlock && isTyping) {
            editedBlock->internalText += (char)wp;
            GenerateCode();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        DeleteObject(fontMain);
        DeleteObject(fontCode);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

//======= 程序入口 ========
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "FinalEditor";
    wc.hCursor = LoadCursor(NULL, IDC_HAND);
    wc.hbrBackground = CreateSolidBrush(RGB(36, 42, 49));
    RegisterClass(&wc);

    HWND hwnd = CreateWindowA("FinalEditor", "专业代码编辑器", 
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,CW_USEDEFAULT,
                             WIN_W, WIN_H, NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
