// Учебная 3D-сцена: Win32/GDI и перспективная проекция.
// Это новый учебный renderer, а не восстановленный renderer Battlefield 2.
// Изучение C++ лучше начать с lesson.h; этот файл отвечает за окно и изображение.
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <string>
#include <vector>
#include "lesson.h"

using lesson::Vec3;
using lesson::dot;
namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr int PANEL = 358;
const COLORREF BACK = RGB(13, 22, 31);
const COLORREF SURFACE = RGB(22, 33, 44);
const COLORREF TEXT = RGB(231, 238, 242);
const COLORREF MUTED = RGB(153, 176, 190);
const COLORREF CYAN = RGB(71, 208, 222);
const COLORREF ORANGE = RGB(247, 176, 82);
const COLORREF GREEN = RGB(112, 220, 157);
const COLORREF PURPLE = RGB(181, 153, 246);

enum Control { Shoot = 101, Restore, Reset, Damage35, Damage80, LookLeft,
               LookRight, FocusTarget, Nearer, Farther, Camera, Source, GroupDamage,
               Preview, TargetOne, TargetTwo, TargetThree, TargetFour, ControlCount };

lesson::World world;
HWND windowHandle = nullptr;
std::array<HWND, ControlCount-101> controls{};
HFONT bodyFont, titleFont, monoFont, smallFont;
HBRUSH panelBrush;
int width = 1280, height = 800, viewWidth = 922;
bool showBasis = false, paused = false;
float cameraYaw = .65f, cameraZoom = 1.0f;
float frameSeconds = 1.0f / 60.0f;

struct CameraFrame { Vec3 position, forward, up, right; float focal; } camera;
struct Face { std::vector<Vec3> points; COLORREF color; float depth; };
std::vector<Face> faces;

Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 subtract(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 scaled(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

void rectangle(HDC dc, int x, int y, int w, int h, COLORREF color)
{
    RECT r{x, y, x + w, y + h};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &r, brush);
    DeleteObject(brush);
}

void text(HDC dc, int x, int y, const std::wstring& value,
          COLORREF color = TEXT, HFONT font = nullptr)
{
    SelectObject(dc, font ? font : bodyFont);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, x, y, value.c_str(), static_cast<int>(value.size()));
}

std::wstring vectorText(Vec3 v)
{
    wchar_t buffer[96];
    swprintf_s(buffer, L"{ %6.2f, %5.2f, %6.2f }", v.x, v.y, v.z);
    return buffer;
}

std::wstring number(float value, int decimals = 2)
{
    wchar_t buffer[64];
    swprintf_s(buffer, L"%.*f", decimals, value);
    return buffer;
}

void prepareCamera()
{
    camera.position = {std::sin(cameraYaw) * 32 * cameraZoom,
                       25 * cameraZoom, -std::cos(cameraYaw) * 32 * cameraZoom};
    camera.forward = subtract(Vec3{0, 0, 1}, camera.position);
    camera.up = {0, 1, 0};
    Vec3 third{};
    // Здесь вызывается настоящий метод OpenBF2 из original/Vec3.cpp.
    third.makeOrthonormalBasis(camera.forward, camera.up);
    camera.right = scaled(third, -1);
    camera.focal = std::min(viewWidth * .97f, height * 1.18f);
}

bool project(Vec3 point, POINT& out)
{
    Vec3 relative = subtract(point, camera.position);
    float depth = dot(relative, camera.forward);
    if (depth < .1f) return false;
    out.x = static_cast<LONG>(viewWidth * .5f + dot(relative, camera.right) * camera.focal / depth);
    out.y = static_cast<LONG>(height * .48f - dot(relative, camera.up) * camera.focal / depth);
    return true;
}

void line3(HDC dc, Vec3 a, Vec3 b, COLORREF color, int thickness = 1, bool dashed = false)
{
    POINT p, q;
    if (!project(a, p) || !project(b, q)) return;
    HPEN pen = CreatePen(dashed ? PS_DOT : PS_SOLID, thickness, color);
    HGDIOBJ previous = SelectObject(dc, pen);
    MoveToEx(dc, p.x, p.y, nullptr);
    LineTo(dc, q.x, q.y);
    SelectObject(dc, previous);
    DeleteObject(pen);
}

void polygon3(HDC dc, const std::vector<Vec3>& vertices, COLORREF color, COLORREF edge)
{
    std::vector<POINT> projected(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i)
        if (!project(vertices[i], projected[i])) return;
    HPEN pen = CreatePen(PS_SOLID, 1, edge);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldPen = SelectObject(dc, pen), oldBrush = SelectObject(dc, brush);
    Polygon(dc, projected.data(), static_cast<int>(projected.size()));
    SelectObject(dc, oldPen); SelectObject(dc, oldBrush);
    DeleteObject(pen); DeleteObject(brush);
}

COLORREF shade(COLORREF color, float factor)
{
    return RGB(std::min(255, static_cast<int>(GetRValue(color)*factor)),
               std::min(255, static_cast<int>(GetGValue(color)*factor)),
               std::min(255, static_cast<int>(GetBValue(color)*factor)));
}

void box(Vec3 center, Vec3 size, COLORREF color)
{
    Vec3 p[8];
    for (int i = 0; i < 8; ++i)
        p[i] = {center.x + ((i&1) ? size.x : -size.x)*.5f,
                center.y + ((i&2) ? size.y : -size.y)*.5f,
                center.z + ((i&4) ? size.z : -size.z)*.5f};
    const int indices[6][4] = {{0,1,3,2},{4,6,7,5},{0,4,5,1},
                               {2,3,7,6},{0,2,6,4},{1,5,7,3}};
    const float light[6] = {.76f, .9f, .6f, 1.15f, .7f, .95f};
    for (int i = 0; i < 6; ++i)
    {
        Face face;
        face.depth = 0;
        face.color = shade(color, light[i]);
        for (int j = 0; j < 4; ++j)
        {
            face.points.push_back(p[indices[i][j]]);
            face.depth += dot(subtract(p[indices[i][j]], camera.position), camera.forward);
        }
        face.depth /= 4;
        faces.push_back(face);
    }
}

void arrow(HDC dc, Vec3 from, Vec3 to, COLORREF color, const wchar_t* label)
{
    POINT a, b;
    if (!project(from, a) || !project(to, b)) return;
    const float dx = static_cast<float>(b.x - a.x), dy = static_cast<float>(b.y - a.y);
    const float length = std::sqrt(dx*dx + dy*dy);
    if (length < 4) return;
    line3(dc, from, to, color, 3);
    const float ux = dx / length, uy = dy / length;
    POINT tip[3] = {{b.x,b.y},
        {static_cast<LONG>(b.x - ux*12 - uy*5), static_cast<LONG>(b.y - uy*12 + ux*5)},
        {static_cast<LONG>(b.x - ux*12 + uy*5), static_cast<LONG>(b.y - uy*12 - ux*5)}};
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldPen = SelectObject(dc, pen), oldBrush = SelectObject(dc, brush);
    Polygon(dc, tip, 3);
    SelectObject(dc, oldPen); SelectObject(dc, oldBrush);
    DeleteObject(pen); DeleteObject(brush);
    if (label) text(dc, b.x + 8, b.y - 12, label, color, smallFont);
}

void label3(HDC dc, Vec3 position, const std::wstring& label, COLORREF color)
{
    POINT p;
    if (project(position, p)) text(dc, p.x + 10, p.y - 10, label, color, smallFont);
}

std::vector<Vec3> viewSector()
{
    const float heading=std::atan2(world.forward.x,world.forward.z);
    const float halfAngle=world.viewAngleDegrees*PI/360;
    std::vector<Vec3> points{{world.player.x,.03f,world.player.z}};
    for(int i=0;i<=48;++i)
    {
        const float angle=heading-halfAngle+2*halfAngle*i/48;
        points.push_back({world.player.x+std::sin(angle)*world.viewDistance,.03f,
                          world.player.z+std::cos(angle)*world.viewDistance});
    }
    return points;
}

void drawWorld(HDC dc)
{
    prepareCamera();
    rectangle(dc, 0, 0, viewWidth, height, BACK);
    polygon3(dc, {{-17,0,-17},{17,0,-17},{17,0,17},{-17,0,17}}, RGB(28,44,47), RGB(42,63,65));

    const lesson::Sight sight=lesson::sightToTarget(world);
    std::vector<Vec3> sector=viewSector();
    polygon3(dc,sector,RGB(26,63,65),RGB(43,115,121));
    for (int i = -12; i <= 12; ++i)
    {
        COLORREF color = i%5 == 0 ? RGB(64,86,90) : RGB(42,62,65);
        line3(dc, {static_cast<float>(i),.02f,-12},{static_cast<float>(i),.02f,12}, color);
        line3(dc, {-12,.02f,static_cast<float>(i)},{12,.02f,static_cast<float>(i)}, color);
    }
    line3(dc, {-12,.025f,0}, {12,.025f,0}, RGB(150,90,89), 2);
    line3(dc, {0,.025f,-12}, {0,.025f,12}, RGB(83,125,169), 2);
    for (int n = -10; n <= 10; n += 5)
    {
        label3(dc, {static_cast<float>(n),.05f,-.2f}, std::to_wstring(n), MUTED);
        if (n != 0) label3(dc, {-.2f,.05f,static_cast<float>(n)}, std::to_wstring(n), MUTED);
    }
    label3(dc, {12.3f,.1f,0}, L"+X", RGB(218,151,145));
    label3(dc, {0,.1f,12.3f}, L"+Z", RGB(137,185,233));

    for(size_t i=1;i<sector.size();++i)line3(dc,sector[i-1],sector[i],CYAN,2);
    line3(dc,sector.back(),sector.front(),CYAN,2);

    faces.clear();
    box({-9,1.0f,-7}, {2.4f,2,2.4f}, RGB(97,115,123));
    box({-6,.55f,-8}, {1.6f,1.1f,1.6f}, RGB(110,126,115));
    box({9,1.3f,7}, {2.0f,2.6f,3.0f}, RGB(97,115,123));
    box({-9,.45f,8}, {2.2f,.9f,1.4f}, RGB(110,126,115));
    box({8,.45f,-8}, {1.4f,.9f,2.0f}, RGB(110,126,115));
    for(std::size_t i=0;i<world.targets.size();++i)
    {
        const Vec3 position=i==world.selectedTarget?world.targetPosition:lesson::targetPositionAt(i);
        box(add(position,{0,.12f,0}),{.85f,.24f,.85f},RGB(106,116,112));
    }
    box(add(world.player, {0,.55f,0}), {.55f,1.1f,.55f}, CYAN);
    box(add(world.player, {0,1.3f,0}), {.38f,.38f,.38f}, RGB(189,228,229));
    std::sort(faces.begin(), faces.end(), [](const Face& a,const Face& b){return a.depth > b.depth;});
    for (const Face& face : faces) polygon3(dc, face.points, face.color, shade(face.color,.72f));

    const std::vector<int> predicted=world.showPrediction?lesson::predictHealth(world):std::vector<int>{};
    for(std::size_t index=0;index<world.targets.size();++index)
    {
        const lesson::Target& target=world.targets[index];
        const bool selected=index==world.selectedTarget;
        const Vec3 position=selected?world.targetPosition:lesson::targetPositionAt(index);
        const COLORREF color=!target.isAlive()?MUTED:(selected?(sight.inSector?GREEN:ORANGE):CYAN);
        line3(dc,add(position,{0,.2f,0}),add(position,{0,1.8f,0}),MUTED,3);
        for(int ring=0;ring<3;++ring)
        {
            const float radius=.75f-ring*.23f;
            std::vector<Vec3> circle;
            for(int i=0;i<48;++i)
            {
                const float angle=2*PI*i/48;
                circle.push_back(add(position,{std::cos(angle)*radius,1.8f+std::sin(angle)*radius,0}));
            }
            polygon3(dc,circle,ring==1?SURFACE:color,shade(color,.8f));
        }
        label3(dc,add(position,{0,3.2f,0}),L"№"+std::to_wstring(index+1)+L"  "+std::to_wstring(target.getHealth())+L" / "+std::to_wstring(target.getMaxHealth()),color);
        if(world.showPrediction)
            label3(dc,add(position,{0,2.65f,0}),L"Прогноз: "+std::to_wstring(predicted[index]),PURPLE);
    }
    line3(dc,add(world.player,{0,.08f,0}),add(world.targetPosition,{0,.08f,0}),GREEN,1,true);
    label3(dc, add(world.player,{0,1.9f,0}), L"ИГРОК", CYAN);

    if(world.shotEffectTime>0)
    {
        const Vec3 end=world.lastShotResult==1?add(world.targetPosition,{0,1.8f,0}):
            add(add(world.player,{0,1.0f,0}),scaled(world.forward,world.viewDistance));
        line3(dc,add(world.player,{0,1.0f,0}),end,world.lastShotResult==1?GREEN:ORANGE,3);
    }

    if (showBasis)
    {
        Vec3 forward = world.forward, up{0,1,0}, third{};
        third.makeOrthonormalBasis(forward,up);
        Vec3 base = add(world.player,{0,.7f,0});
        arrow(dc,base,add(base,scaled(forward,2)),GREEN,L"forward");
        arrow(dc,base,add(base,scaled(up,2)),PURPLE,L"up");
        arrow(dc,base,add(base,scaled(third,2)),RGB(234,140,132),L"third");
    }
    else
    {
        Vec3 base = add(world.player,{0,.7f,0});
        arrow(dc,base,add(base,scaled(world.velocity,.85f)),GREEN,L"velocity");
        arrow(dc,base,add(base,scaled(world.forward,3)),PURPLE,L"forward");
    }

    text(dc,22,20,L"УЧЕБНЫЙ ТИР",TEXT,titleFont);
    text(dc,22,53,L"УРОК 4  /  конструкторы, std::vector и цикл for",MUTED,smallFont);
    rectangle(dc,16,height-76,viewWidth-32,60,RGB(18,30,40));
    text(dc,28,height-66,L"1–4 — выбрать мишень     P — прогноз на копиях     X — нанести урон всем",TEXT,smallFont);
    text(dc,28,height-42,L"J — выстрел     H — новые мишени     WASD — движение     Q/E/F — взгляд",MUTED,smallFont);
    if (paused) text(dc,22,86,L"ПАУЗА",ORANGE);
}

void drawPanel(HDC dc)
{
    const int x = viewWidth + 22;
    rectangle(dc,viewWidth,0,PANEL,height,SURFACE);
    text(dc,x,22,L"Объекты, копии и ссылки",TEXT,titleFont);
    text(dc,x,53,L"constructor + std::vector + for",MUTED,smallFont);
    const int health=activeTarget(world).getHealth();
    const int maximum=activeTarget(world).getMaxHealth();
    const bool alive=activeTarget(world).isAlive();
    text(dc,x,132,L"targets["+std::to_wstring(world.selectedTarget)+L"].getHealth()",MUTED,monoFont);
    text(dc,x,154,std::to_wstring(health)+L" / "+std::to_wstring(maximum)+L" HP",alive?GREEN:ORANGE,titleFont);
    rectangle(dc,x,183,312,10,RGB(43,58,67));
    if(health>0&&maximum>0)rectangle(dc,x,183,312*health/maximum,10,GREEN);
    text(dc,x,213,alive?L"isAlive() = true":L"isAlive() = false",TEXT,monoFont);
    std::wstring shot=L"Выбери действие";
    if(world.lastShotResult==1)shot=std::to_wstring(world.healthBeforeShot)+L" → "+std::to_wstring(health)+L" (−"+std::to_wstring(world.lastAppliedDamage)+L" HP)";
    if(world.lastShotResult==2)shot=L"Промах: мишень вне сектора";
    if(world.lastShotResult==3)shot=L"Мишень уже уничтожена";
    if(world.lastShotResult==4)shot=L"Групповой урон применён";
    if(world.showPrediction)
    {
        const int prediction=lesson::predictHealth(world)[world.selectedTarget];
        shot=L"Прогноз: "+std::to_wstring(health)+L" → "+std::to_wstring(prediction);
        if(maximum>0&&prediction>0)rectangle(dc,x,196,312*prediction/maximum,4,PURPLE);
    }
    text(dc,x,244,shot,world.showPrediction?PURPLE:TEXT);
    text(dc,x,268,world.showPrediction?L"Здоровье объектов не меняется":L"targets.size() = "+std::to_wstring(world.targets.size()),MUTED,smallFont);
    text(dc,x,295,L"Выбранный урон: "+std::to_wstring(world.shotDamage),TEXT);
    text(dc,x,632,L"Дальность: "+number(world.viewDistance,0)+L" м",TEXT);
    const lesson::Sight sight=lesson::sightToTarget(world);
    text(dc,x,676,sight.directionDefined?L"dot = "+number(sight.alignment,3)+L" / порог "+number(sight.threshold,3):L"Направление не определено",MUTED,smallFont);
    text(dc,x,699,!alive?L"МИШЕНЬ УНИЧТОЖЕНА":(sight.inSector?L"МИШЕНЬ В СЕКТОРЕ":L"МИШЕНЬ ВНЕ СЕКТОРА"),alive&&sight.inSector?GREEN:ORANGE,smallFont);
}

void redraw(HDC target)
{
    HDC memory = CreateCompatibleDC(target);
    HBITMAP bitmap = CreateCompatibleBitmap(target,width,height);
    HGDIOBJ previous = SelectObject(memory,bitmap);
    HRGN clip = CreateRectRgn(0,0,viewWidth,height);
    SelectClipRgn(memory,clip);
    drawWorld(memory);
    SelectClipRgn(memory,nullptr);
    DeleteObject(clip);
    drawPanel(memory);
    BitBlt(target,0,0,width,height,memory,0,0,SRCCOPY);
    SelectObject(memory,previous);
    DeleteObject(bitmap); DeleteDC(memory);
}

void updateLabels()
{
    SetWindowTextW(controls[Preview-101],world.showPrediction?L"[x] Предпросмотр на копиях [P]":L"[ ] Предпросмотр на копиях [P]");
    for(HWND control:controls) InvalidateRect(control,nullptr,FALSE);
}

void arrangeControls()
{
    const int x = viewWidth + 22;
    for(int i=0;i<4;++i)MoveWindow(controls[TargetOne-101+i],x+i*80,86,72,32,TRUE);
    MoveWindow(controls[Damage35-101],x,318,150,32,TRUE);
    MoveWindow(controls[Damage80-101],x+162,318,150,32,TRUE);
    MoveWindow(controls[Shoot-101],x,362,312,32,TRUE);
    MoveWindow(controls[GroupDamage-101],x,406,312,32,TRUE);
    MoveWindow(controls[Preview-101],x,450,312,32,TRUE);
    MoveWindow(controls[Restore-101],x,494,150,32,TRUE);
    MoveWindow(controls[Reset-101],x+162,494,150,32,TRUE);
    MoveWindow(controls[LookLeft-101],x,540,150,32,TRUE);
    MoveWindow(controls[LookRight-101],x+162,540,150,32,TRUE);
    MoveWindow(controls[FocusTarget-101],x,584,312,32,TRUE);
    MoveWindow(controls[Nearer-101],x+226,626,38,36,TRUE);
    MoveWindow(controls[Farther-101],x+274,626,38,36,TRUE);
    MoveWindow(controls[Camera-101],x,728,150,32,TRUE);
    MoveWindow(controls[Source-101],x+162,728,150,32,TRUE);
}

void command(int id)
{
    switch(id)
    {
    case Shoot:if(!paused)lesson::shoot(world);break;
    case GroupDamage:if(!paused)lesson::applyDamageToAll(world);break;
    case Preview:world.showPrediction=!world.showPrediction;break;
    case TargetOne:case TargetTwo:case TargetThree:case TargetFour:
        lesson::selectTarget(world,static_cast<std::size_t>(id-TargetOne));break;
    case Restore:lesson::restoreTargets(world);break;
    case Reset:world=lesson::World{};paused=false;break;
    case Damage35:world.shotDamage=35;break;
    case Damage80:world.shotDamage=80;break;
    case Camera: cameraYaw=.65f;cameraZoom=1;break;
    case LookLeft:lesson::rotateLook(world,-15);break;
    case LookRight:lesson::rotateLook(world,15);break;
    case Nearer:world.viewDistance=std::max(1.0f,world.viewDistance-1);break;
    case Farther:world.viewDistance=std::min(15.0f,world.viewDistance+1);break;
    case FocusTarget:lesson::lookAtTarget(world);break;
    case Source:
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr,path,MAX_PATH);
        std::wstring source(path);
        source = L"\"" + source.substr(0,source.find_last_of(L"\\/")+1)+L"lesson.h\"";
        ShellExecuteW(windowHandle,L"open",L"notepad.exe",source.c_str(),nullptr,SW_SHOWNORMAL);
        break;
    }
    }
    updateLabels();
    arrangeControls();
    SetFocus(windowHandle);
    InvalidateRect(windowHandle,nullptr,FALSE);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
    {
        windowHandle=hwnd;
        const wchar_t* captions[]={L"Выстрел по выбранной [J]",L"Новые мишени [H]",L"Сброс [R]",L"Урон 35",L"Урон 80",
            L"← Взгляд [Q]",L"Взгляд [E] →",L"Смотреть на мишень [F]",L"−",L"+",L"Камера",L"Циклы [F2]",
            L"Нанести урон всем [X]",L"",L"№1 [1]",L"№2 [2]",L"№3 [3]",L"№4 [4]"};
        for(int i=0;i<ControlCount-101;++i)
        {
            controls[i]=CreateWindowW(L"BUTTON",captions[i],WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
                0,0,10,10,hwnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(101+i)),nullptr,nullptr);
            SendMessageW(controls[i],WM_SETFONT,reinterpret_cast<WPARAM>(bodyFont),TRUE);
        }
        updateLabels();
        SetTimer(hwnd,1,16,nullptr);
        return 0;
    }
    case WM_SIZE:
        width=LOWORD(lParam);height=HIWORD(lParam);viewWidth=std::max(100,width-PANEL);
        arrangeControls();return 0;
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize={1040,800};return 0;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;HDC dc=BeginPaint(hwnd,&paint);redraw(dc);EndPaint(hwnd,&paint);return 0;
    }
    case WM_DRAWITEM:
    {
        auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        const bool selected=(item->itemState&ODS_SELECTED)!=0;
        bool enabled=(item->CtlID==Damage35&&world.shotDamage==35)||(item->CtlID==Damage80&&world.shotDamage==80)
                    ||(item->CtlID==Preview&&world.showPrediction)
                    ||(item->CtlID>=TargetOne&&item->CtlID<=TargetFour&&item->CtlID-TargetOne==world.selectedTarget);
        COLORREF bg=selected?RGB(57,78,91):(enabled?RGB(42,75,85):RGB(35,51,65));
        rectangle(item->hDC,0,0,item->rcItem.right,item->rcItem.bottom,bg);
        wchar_t label[128];GetWindowTextW(item->hwndItem,label,128);
        SelectObject(item->hDC,bodyFont);SetTextColor(item->hDC,TEXT);SetBkMode(item->hDC,TRANSPARENT);
        RECT r=item->rcItem;DrawTextW(item->hDC,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        return TRUE;
    }
    case WM_COMMAND:command(LOWORD(wParam));return 0;
    case WM_KEYDOWN:
        if(lParam&(1LL<<30))return 0;
        switch(wParam)
        {
        case 'J':command(Shoot);break;case 'H':command(Restore);break;case 'R':command(Reset);break;
        case 'X':command(GroupDamage);break;case 'P':command(Preview);break;
        case '1':command(TargetOne);break;case '2':command(TargetTwo);break;
        case '3':command(TargetThree);break;case '4':command(TargetFour);break;
        case 'B':showBasis=!showBasis;break;case VK_F2:command(Source);break;
        case 'Q':command(LookLeft);break;case 'E':command(LookRight);break;
        case 'F':command(FocusTarget);break;
        case VK_SPACE:paused=!paused;break;
        case VK_ESCAPE:PostMessage(hwnd,WM_CLOSE,0,0);break;
        }return 0;
    case WM_LBUTTONDOWN:if(GET_X_LPARAM(lParam)<viewWidth)command(Shoot);return 0;
    case WM_RBUTTONDOWN:SetCapture(hwnd);return 0;
    case WM_RBUTTONUP:ReleaseCapture();return 0;
    case WM_MOUSEMOVE:
    {
        static int previousX=0;
        int x=GET_X_LPARAM(lParam);
        if((wParam&MK_RBUTTON)&&x<viewWidth)cameraYaw+=(x-previousX)*.007f;
        previousX=x;return 0;
    }
    case WM_MOUSEWHEEL:
        cameraZoom=std::max(.6f,std::min(1.5f,cameraZoom-GET_WHEEL_DELTA_WPARAM(wParam)/120*.08f));return 0;
    case WM_TIMER:
    {
        static auto previous=std::chrono::steady_clock::now();
        auto now=std::chrono::steady_clock::now();
        frameSeconds=std::min(.05f,std::chrono::duration<float>(now-previous).count());previous=now;
        if(!paused && !IsIconic(hwnd) && GetForegroundWindow()==hwnd)
        {
            const auto down=[](int key){return (GetAsyncKeyState(key)&0x8000)!=0;};
            float x=static_cast<float>(down('D')||down(VK_RIGHT))-static_cast<float>(down('A')||down(VK_LEFT));
            float z=static_cast<float>(down('W')||down(VK_UP))-static_cast<float>(down('S')||down(VK_DOWN));
            if(x!=0||z!=0)
            {
                if(world.autoWalk){world.autoWalk=false;updateLabels();}
            }
            const bool wasWalking=world.autoWalk;
            lesson::updatePlayer(world,x,z,frameSeconds);
            world.shotEffectTime=std::max(0.0f,world.shotEffectTime-frameSeconds);
            if(wasWalking!=world.autoWalk)updateLabels();
        }
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
    }
    case WM_DESTROY:KillTimer(hwnd,1);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,message,wParam,lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show)
{
    SetProcessDPIAware();
    bodyFont=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    smallFont=CreateFontW(-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    titleFont=CreateFontW(-20,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    monoFont=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Consolas");
    panelBrush=CreateSolidBrush(SURFACE);
    WNDCLASSW cls{};cls.hInstance=instance;cls.lpfnWndProc=windowProc;
    cls.lpszClassName=L"OpenBF2CollectionLab";cls.hCursor=LoadCursor(nullptr,IDC_ARROW);
    cls.hIcon=LoadIcon(nullptr,IDI_APPLICATION);RegisterClassW(&cls);
    HWND hwnd=CreateWindowW(cls.lpszClassName,L"Урок 04 — Конструкторы, vector и циклы",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT,CW_USEDEFAULT,1280,840,nullptr,nullptr,instance,nullptr);
    if(!hwnd)return 1;
    ShowWindow(hwnd,show);UpdateWindow(hwnd);
    MSG message{};
    while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
    DeleteObject(bodyFont);DeleteObject(smallFont);DeleteObject(titleFont);DeleteObject(monoFont);DeleteObject(panelBrush);
    return static_cast<int>(message.wParam);
}
