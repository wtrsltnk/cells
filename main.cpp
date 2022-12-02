/*
 * File:   main.cpp
 * Author: Wouter
 *
 * Created on April 8, 2009, 8:29 PM
 */

#include "opengl.h"

#include "stb_truetype.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <chrono>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>
#include <sqlitelib.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

int running = true; // Flag telling if the program is running

static float fontSize = 16.0f;
unsigned char ttf_buffer[1 << 20];
unsigned char temp_bitmap[512 * 512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GLuint ftex;

void my_stbtt_initfont()
{
    FILE *f = nullptr;

    errno_t err = fopen_s(&f, "c:/windows/fonts/SourceCodePro-Regular.ttf", "rb");
    if (err)
    {
        err = fopen_s(&f, "c:/windows/fonts/consola.ttf", "rb");
    }

    if (err)
    {
        spdlog::error("loading font failed");

        return;
    }

    fread(ttf_buffer, 1, 1 << 20, f);
    stbtt_BakeFontBitmap(ttf_buffer, 0, fontSize, temp_bitmap, 512, 512, 32, 96, cdata); // no guarantee this fits!
    // can free ttf_buffer at this point
    glActiveTextureARB(GL_TEXTURE0);
    glGenTextures(1, &ftex);
    glBindTexture(GL_TEXTURE_2D, ftex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
    // can free temp_bitmap at this point
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    fclose(f);
}

float my_stbtt_print_width(
    const std::string &text)
{
    const char *txt = text.c_str();

    float x = 0;
    float y = 0;

    while (*txt)
    {
        if (*txt == '\n')
        {
            break;
        }

        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cdata, 512, 512, *txt - 32, &x, &y, &q, 1);

        ++txt;
    }

    return x;
}

void my_stbtt_print(
    float x,
    float y,
    const std::string &text,
    const glm::vec4 &color)
{
    const char *txt = text.c_str();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);

    glActiveTextureARB(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ftex);

    glBegin(GL_QUADS);
    glColor4f(color.r, color.g, color.b, color.a);

    while (*txt)
    {
        if (*txt == '\n')
        {
            break;
        }

        stbtt_aligned_quad q;

        if (*txt == '\t')
        {
            for (int i = 0; i < 4; i++)
            {
                stbtt_GetBakedQuad(cdata, 512, 512, '0' - 32, &x, &y, &q, 1);
            }

            ++txt;
            continue;
        }

        if (*txt < 32)
        {
            stbtt_GetBakedQuad(cdata, 512, 512, '0' - 32, &x, &y, &q, 1);

            ++txt;
            continue;
        }

        stbtt_GetBakedQuad(cdata, 512, 512, *txt - 32, &x, &y, &q, 1);
        glTexCoord2f(q.s0, q.t0);
        glVertex2f(q.x0, q.y0);

        glTexCoord2f(q.s1, q.t0);
        glVertex2f(q.x1, q.y0);

        glTexCoord2f(q.s1, q.t1);
        glVertex2f(q.x1, q.y1);

        glTexCoord2f(q.s0, q.t1);
        glVertex2f(q.x0, q.y1);

        ++txt;
    }
    glEnd();
    glActiveTextureARB(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
}

class Console
{
public:
    Console();

    void OpenFile(
        const std::string &filename);

    void Resize(
        float width,
        float height);

    void Render();

    void AddChar(
        unsigned int c);

    void Backspace();

    void Enter();

    void Scroll(
        float amount);

    void ScrollTo(
        int position);

    float Margin() const { return _margin; }

private:
    std::vector<std::string> _lines;
    std::string _input;
    float _scroll = 0;
    float _margin = 30;
    std::vector<char> _fileData;
    std::vector<size_t> _lineStarts;
    float _sbStart = 100;
    float _sbHeight = 100;
    float _width = 0, _height = 0;

    float LineHeight() const
    {
        return fontSize * 1.2f;
    }

    float ContentHeight() const
    {
        return _lineStarts.size() * LineHeight();
    }

    void SetScroll(
        float scroll);

    void RecalcScrolBar();
};

Console::Console()
{
    _lines.push_back("hit ` to toggle this console");
}

void Console::OpenFile(
    const std::string &filename)
{
    std::ifstream ifs(filename, std::ifstream::in);

    char c = ifs.get();

    while (ifs.good())
    {
        if (c == '\n')
        {
            _lineStarts.push_back(_fileData.size());
        }

        _fileData.push_back(c);

        c = ifs.get();
    }

    ifs.close();

    RecalcScrolBar();
}

void Console::SetScroll(
    float scroll)
{
    _scroll = scroll;

    if (_scroll > 0)
    {
        _scroll = 0;
    }

    auto contentHeight = ContentHeight() - LineHeight();
    if (_scroll < -contentHeight)
    {
        _scroll = -contentHeight;
    }

    RecalcScrolBar();
}

void Console::Scroll(
    float amount)
{
    SetScroll(_scroll + (amount * 3) * LineHeight());
}

void Console::ScrollTo(
    int position)
{
    auto aspect = ((position - _sbHeight / 2.0f) / _height);

    SetScroll(-(ContentHeight() * aspect));
}

void Console::RecalcScrolBar()
{
    auto contentHeight = ContentHeight();
    auto sbAspect = _height / contentHeight;
    _sbHeight = _height * sbAspect;
    _sbStart = -_scroll * sbAspect;
}

void Console::Resize(
    float width,
    float height)
{
    _width = width;
    _height = height;

    RecalcScrolBar();
}

void Console::Render()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);

    glActiveTextureARB(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    auto textHeight = (_lineStarts.size() + 1) * LineHeight();
    auto lineNumberWidth = my_stbtt_print_width(std::to_string(_lineStarts.size() * 10));

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glVertex2f(lineNumberWidth, 0.0f);
    glVertex2f(_width - _margin, 0.0f);
    glVertex2f(_width - _margin, _height);
    glVertex2f(lineNumberWidth, _height);
    glEnd();

    glTranslatef(0, _scroll, 0);

    auto textColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    auto lineNumberColor = glm::vec4(0.1f, 0.4f, 0.8f, 1.0f);

    float y = 0;
    int lineIndex = 1;
    size_t prevLineStart = -1;
    for (auto line = _lineStarts.begin(); line != _lineStarts.end(); line++, lineIndex++)
    {
        y += LineHeight();

        if ((y + LineHeight() + LineHeight()) < -_scroll)
        {
            continue;
        }

        if ((y - LineHeight()) > -(_scroll - _height))
        {
            continue;
        }

        auto lineNumber = fmt::format("{}", lineIndex);
        my_stbtt_print(lineNumberWidth - my_stbtt_print_width(lineNumber) - 5, y, lineNumber, lineNumberColor);

        std::string s(_fileData.data() + prevLineStart + 1, _fileData.data() + *line);
        prevLineStart = *line;
        my_stbtt_print(10 + lineNumberWidth, y, s, textColor);
    }

    glLoadIdentity();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);

    glActiveTextureARB(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.6f, 1.0f, 0.5f);
    glVertex2f(_width, _sbStart + _sbHeight);
    glVertex2f(_width - _margin, _sbStart + _sbHeight);
    glVertex2f(_width - _margin, _sbStart);
    glVertex2f(_width, _sbStart);
    glEnd();
}

void Console::AddChar(
    unsigned int c)
{
    _input += c;
}

void Console::Backspace()
{
    if (_input.empty())
    {
        return;
    }

    _input = _input.substr(0, _input.size() - 1);
}

void Console::Enter()
{
    if (_input == "exit" || _input == "quit")
    {
        _lines.push_back("bye!");
        running = false;
        _input = "";
        return;
    }

    _lines.push_back(_input);
    _input = "";
}

static Console console;

void CharCallback(
    GLFWwindow *window,
    unsigned int codepoint)
{
    (void)window;

    console.AddChar(codepoint);
}

int active_cell_col = 0, active_cell_row = 0;

void MoveSelectionLeft()
{
    active_cell_col--;

    if (active_cell_col < 0)
    {
        active_cell_col = 0;
    }
}

void MoveSelectionRight()
{
    active_cell_col++;
}

void MoveSelectionUp()
{
    active_cell_row--;

    if (active_cell_row < 0)
    {
        active_cell_row = 0;
    }
}

void MoveSelectionDown()
{
    active_cell_row++;
}

void KeyCallback(
    GLFWwindow *window,
    int key,
    int scancode,
    int action,
    int mods)
{
    (void)window;
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        console.Backspace();
    }
    else if (key == GLFW_KEY_ENTER && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        console.Enter();
    }
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
    {
        running = false;
    }

    if ((key == GLFW_KEY_LEFT || (key == GLFW_KEY_TAB && mods & GLFW_MOD_SHIFT)) && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        MoveSelectionLeft();
    }
    else if ((key == GLFW_KEY_RIGHT || key == GLFW_KEY_TAB) && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        MoveSelectionRight();
    }
    else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        MoveSelectionUp();
    }
    else if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        MoveSelectionDown();
    }
}

void ScrollCallback(
    GLFWwindow *window,
    double xoffset,
    double yoffset)
{
    console.Scroll(float(yoffset));
}

int w = 1024, h = 768;

void ResizeCallback(
    GLFWwindow *window,
    int width,
    int height)
{
    w = width;
    h = height;

    console.Resize(float(w), float(h));
}

static bool sbDragging = false;

void MouseButtonCallback(
    GLFWwindow *window,
    int button,
    int action,
    int mods)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (x >= w - console.Margin() && x < w)
    {
        console.ScrollTo(int(y));

        sbDragging = (action == GLFW_PRESS);
    }

    if (action == GLFW_RELEASE)
    {
        sbDragging = false;
    }
}

void CursorPosCallback(
    GLFWwindow *window,
    double x,
    double y)
{
    if (sbDragging)
    {
        console.ScrollTo(int(y));
    }
}

std::unique_ptr<sqlitelib::Sqlite> InitDb()
{
    auto db = std::make_unique<sqlitelib::Sqlite>("./test.db");

    try
    {
        db->execute(R"(
  CREATE TABLE IF NOT EXISTS cells (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    col INTEGER,
    row INTEGER,
    function TEXT,
    tmp_value TEXT,
    sheet INTEGER
  )
)");

        db->execute(R"(
  CREATE TABLE IF NOT EXISTS cols (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    col_index INTEGER,
    size INTEGER
  )
)");

        db->execute(R"(
  CREATE TABLE IF NOT EXISTS rows (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    row_index INTEGER,
    size INTEGER
  )
)");

        db->execute(R"(
  CREATE TABLE IF NOT EXISTS sheets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT
  )
)");

        db->execute(R"(DELETE FROM cols;)");
        db->execute(R"(INSERT INTO cols (col_index, size) VALUES (3, 140);)");
        db->execute(R"(INSERT INTO cols (col_index, size) VALUES (5, 300);)");
    }
    catch (const std::exception &ex)
    {
        std::cout << db->errormsg() << std::endl;
    }

    return db;
}

std::pair<int, int> pair_from_tuple(const std::tuple<int, int> &tuple)
{
    return std::make_pair(std::get<0>(tuple), std::get<1>(tuple));
}

#define MAX

std::string columnIndexToLetters(int n)
{
    std::string str; // To store result (Excel column name)
    int i = 0;       // To store current index in str which is result

    while (n > 0)
    {
        // Find remainder
        int rem = n % 26;

        // If remainder is 0, then a 'Z' must be there in output
        if (rem == 0)
        {
            str += 'Z';
            n = (n / 26) - 1;
        }
        else // If remainder is non-zero
        {
            str += (rem - 1) + 'A';
            n = n / 26;
        }
    }

    std::reverse(str.begin(), str.end());
    return str;
}

void renderSheet(
    std::unique_ptr<sqlitelib::Sqlite> &db,
    int atx,
    int aty)
{
    glDisable(GL_DEPTH_TEST);
    try
    {
        glViewport(atx, 0, w - atx, h - aty);

        auto cols = db->execute<int, int>("SELECT col_index, size FROM cols");
        auto rows = db->execute<int, int>("SELECT row_index, size FROM rows");
        auto cells = db->execute<int, int, std::string>("SELECT col, row, tmp_value FROM cells");

        std::map<int, int> cols_map, rows_map;

        for (auto const &e : cols)
        {
            cols_map.insert(pair_from_tuple(e));
        }

        for (auto const &e : rows)
        {
            rows_map.insert(pair_from_tuple(e));
        }

        const int defaultcol_w = 100, defaultcol_h = 30;
        const int header_w = 40, header_h = 30;
        int i = 0, x = header_w, y = header_h + 1;
        glBegin(GL_QUADS);
        glColor3f(0.85f, 0.85f, 0.85f);

        glVertex2f(0.0f, 0.0f);
        glVertex2f(w, 0.0f);
        glVertex2f(w, header_h);
        glVertex2f(0.0f, header_h);

        glVertex2f(0.0f, 0.0f);
        glVertex2f(header_w, 0.0f);
        glVertex2f(header_w, h);
        glVertex2f(0.0f, h);
        glEnd();

        glBegin(GL_LINES);
        glColor3f(0.79f, 0.79f, 0.79f);
        auto selected_x = 0, selected_y = 0;
        auto selected_w = 0, selected_h = 0;
        while (x < w)
        {
            if (i == active_cell_col)
            {
                selected_x = x;
                selected_w = defaultcol_w;
            }

            glVertex2f(float(x), 0.0f);
            glVertex2f(float(x), float(h));

            auto colw = cols_map.find(i);
            if (colw != cols_map.end())
            {
                if (i == active_cell_col)
                {
                    selected_w = colw->second;
                }

                x += colw->second;
            }
            else
            {
                x += defaultcol_w;
            }

            i++;
        }

        i = 0;
        while (y < h)
        {
            if (i == active_cell_row)
            {
                selected_y = y;
                selected_h = defaultcol_h;
            }

            glVertex2f(0.0f, float(y));
            glVertex2f(float(w), float(y));

            auto rowh = rows_map.find(i);
            if (rowh != rows_map.end())
            {
                if (i == active_cell_row)
                {
                    selected_h = rowh->second;
                }

                y += rowh->second;
            }
            else
            {
                y += defaultcol_h;
            }

            i++;
        }
        glEnd();

        glBegin(GL_LINES);
        glColor3f(0.4f, 0.55f, 0.65f);

        glVertex2i(selected_x, selected_y);
        glVertex2f(selected_x + selected_w, selected_y);

        glVertex2f(selected_x + selected_w, selected_y);
        glVertex2f(selected_x + selected_w, selected_y + selected_h);

        glVertex2f(selected_x + selected_w, selected_y + selected_h);
        glVertex2f(selected_x, selected_y + selected_h);

        glVertex2f(selected_x, selected_y + selected_h);
        glVertex2f(selected_x, selected_y);

        glVertex2f(selected_x + 1, selected_y + 1);
        glVertex2f(selected_x + selected_w - 1, selected_y + 1);

        glVertex2f(selected_x + selected_w - 1, selected_y + 1);
        glVertex2f(selected_x + selected_w - 1, selected_y + selected_h - 1);

        glVertex2f(selected_x + selected_w - 1, selected_y + selected_h - 1);
        glVertex2f(selected_x + 1, selected_y + selected_h - 1);

        glVertex2f(selected_x + 1, selected_y + selected_h - 1);
        glVertex2f(selected_x + 1, selected_y + 1);

        glEnd();
        glLineWidth(1.0f);

        i = 0, x = header_w, y = 0;
        while (x < w)
        {
            auto fromx = x;

            auto colw = cols_map.find(i);
            if (colw != cols_map.end())
            {
                x += colw->second;
            }
            else
            {
                x += defaultcol_w;
            }

            i++;

            auto fpsstr = columnIndexToLetters(i);
            auto strwidth = my_stbtt_print_width(fpsstr);

            my_stbtt_print(
                fromx + ((x - fromx) / 2.0f) - (strwidth / 2.0f),
                fontSize * 1.2f,
                fpsstr,
                glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        i = 0, x = 0, y = header_h;
        while (y < h)
        {
            glVertex2f(0.0f, float(y));
            glVertex2f(float(w), float(y));

            auto fromy = y;
            auto rowh = rows_map.find(i);
            if (rowh != rows_map.end())
            {
                y += rowh->second;
            }
            else
            {
                y += defaultcol_h;
            }

            i++;

            auto fpsstr = fmt::format("{}", i);
            auto strwidth = my_stbtt_print_width(fpsstr);

            my_stbtt_print(
                (header_w / 2.0f) - (strwidth / 2.0f),
                fromy + (fontSize * 1.2f),
                fpsstr,
                glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << db->errormsg() << std::endl;
    }
}

int main(
    int argc,
    char *argv[])
{
    if (argc > 1)
    {
        console.OpenFile(argv[1]);
    }

    auto db = InitDb();

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    auto window = glfwCreateWindow(
        w, h,
        "Power Cells",
        nullptr,
        nullptr);

    // If we could not open a window, exit now
    if (!window)
    {
        glfwTerminate();

        return 0;
    }

    glfwSetCharCallback(window, CharCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);

    glfwMakeContextCurrent(window);

    gladLoadGL();

    my_stbtt_initfont();

    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    console.Resize(float(w), float(h));

    double time = glfwGetTime();
    double prevTime = time;
    int fps = 0;
    double realFps = 0;

    // Main rendering loop
    while (running && !glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        fps++;
        double newTime = glfwGetTime();
        double timeDiff = newTime - prevTime;
        prevTime = newTime;
        if ((newTime - time) > 1)
        {
            realFps = double(fps) / (newTime - time);

            fps = 0;
            time = newTime;
        }

        double mx = 0, my = h / 2;
        glfwGetCursorPos(window, &mx, &my);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, w, h);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glOrtho(0, w, h, 0, -0.2f, 1.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // console.Render();

        renderSheet(db, 0, 0);

        glViewport(0, 0, w, h);

        glLoadIdentity();
        auto fpsstr = fmt::format("fps: {:.2f}", realFps);

        my_stbtt_print(
            w - my_stbtt_print_width(fpsstr) - (fontSize * 0.4f) - console.Margin(),
            fontSize * 1.2f,
            fpsstr,
            glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        // Swap front and back buffers (we use a double buffered display)
        glfwSwapBuffers(window);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(5ms);
    }

    glfwTerminate();

    return (EXIT_SUCCESS);
}
