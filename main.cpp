/*
 * File:   main.cpp
 * Author: Wouter
 *
 * Created on April 8, 2009, 8:29 PM
 */

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include "stb_truetype.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <chrono>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <map>
#include <numeric> // for accumelate
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

void CharCallback(
    GLFWwindow *window,
    unsigned int codepoint)
{
    (void)window;
    (void)codepoint;
}

int active_cell_col = 0, active_cell_row = 0;
int scroll_cols = 0, max_visible_col_count = 0, scroll_rows = 0, max_visible_row_count = 0;
const int defaultcell_w = 100, defaultcell_h = 30;
const int input_line_h = 50, header_w = 40, header_h = 30;
const float padding = 10.0f;
const float cell_padding = 2.0f;

int w = 1024, h = 768;

static std::unique_ptr<sqlitelib::Sqlite> db;

std::string columnIndexToLetters(int n)
{
    std::string str; // To store result (Excel column name)

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

void EnsureSelectionInView()
{
    {
        auto cols = db->execute<int, int>("SELECT col_index, size FROM cols");
        std::map<int, int> col_widths;
        for (auto const &col : cols)
        {
            col_widths[std::get<0>(col)] = defaultcell_w + std::get<1>(col);
        }
        for (int i = 0; i <= active_cell_col; i++)
        {
            if (col_widths.count(i) > 0)
            {
                continue;
            }
            col_widths[i] = defaultcell_w;
        }

        auto total_col_width = std::accumulate(
            col_widths.begin(),
            col_widths.end(),
            0,
            [](int value, const std::map<int, int>::value_type &p) { return value + p.second; });

        auto min_scroll_cols = 0;

        total_col_width += header_w;
        while (total_col_width > w)
        {
            total_col_width -= col_widths[min_scroll_cols];
            min_scroll_cols++;
        }

        if (scroll_cols < min_scroll_cols)
        {
            scroll_cols = min_scroll_cols;
        }
        else if (scroll_cols > active_cell_col)
        {
            scroll_cols = active_cell_col;
        }

        max_visible_col_count = 0;
        int x = header_w;
        while (x < w)
        {
            if (col_widths.count(scroll_cols + max_visible_col_count) == 1)
            {
                x += col_widths[scroll_cols + max_visible_col_count];
            }
            else
            {
                x += defaultcell_w;
            }
            max_visible_col_count++;
        }
    }

    {
        auto rows = db->execute<int, int>("SELECT row_index, size FROM rows");
        std::map<int, int> row_widths;
        for (auto const &row : rows)
        {
            row_widths[std::get<0>(row)] = defaultcell_h + std::get<1>(row);
        }
        for (int i = 0; i <= active_cell_row; i++)
        {
            if (row_widths.count(i) > 0)
            {
                continue;
            }
            row_widths[i] = defaultcell_h;
        }

        auto total_row_size = std::accumulate(
            row_widths.begin(),
            row_widths.end(),
            0,
            [](int value, const std::map<int, int>::value_type &p) { return value + p.second; });

        auto min_scroll_rows = 0;

        total_row_size += input_line_h + header_h;
        while (total_row_size > h)
        {
            total_row_size -= row_widths[min_scroll_rows];
            min_scroll_rows++;
        }

        if (scroll_rows < min_scroll_rows)
        {
            scroll_rows = min_scroll_rows;
        }
        else if (scroll_rows > active_cell_row)
        {
            scroll_rows = active_cell_row;
        }

        max_visible_row_count = 0;
        int y = input_line_h + header_h;
        while (y < h)
        {
            if (row_widths.count(scroll_rows + max_visible_row_count) == 1)
            {
                y += row_widths[scroll_rows + max_visible_row_count];
            }
            else
            {
                y += defaultcell_h;
            }
            max_visible_row_count++;
        }
    }
}

void MoveSelectionLeft()
{
    active_cell_col--;

    if (active_cell_col < 0)
    {
        active_cell_col = 0;
    }

    EnsureSelectionInView();
}

void MoveSelectionRight()
{
    active_cell_col++;

    EnsureSelectionInView();
}

void MoveSelectionUp()
{
    active_cell_row--;

    if (active_cell_row < 0)
    {
        active_cell_row = 0;
    }

    EnsureSelectionInView();
}

void MoveSelectionDown()
{
    active_cell_row++;

    EnsureSelectionInView();
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
    }
    else if (key == GLFW_KEY_ENTER && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
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
    (void)window;

    if (xoffset < 0)
    {
        scroll_cols++;
    }
    else if (xoffset > 0)
    {
        scroll_cols--;
    }
    else if (yoffset < 0)
    {
        scroll_rows++;
    }
    else if (yoffset > 0)
    {
        scroll_rows--;
    }

    if (scroll_rows < 0)
    {
        scroll_rows = 0;
    }

    if (scroll_cols < 0)
    {
        scroll_cols = 0;
    }
}

void ResizeCallback(
    GLFWwindow *window,
    int width,
    int height)
{
    (void)window;

    w = width;
    h = height;
}

std::pair<int, int> pair_from_tuple(
    const std::tuple<int, int> &tuple)
{
    return std::make_pair(std::get<0>(tuple), std::get<1>(tuple));
}

std::map<int, int> select_into_map(
    const char *query)
{
    auto cols = db->execute<int, int>(query);

    std::map<int, int> map;

    for (auto const &e : cols)
    {
        map.insert(pair_from_tuple(e));
    }

    return map;
}

bool IsHoveringInputLine(
    int x,
    int y)
{
    auto strwidth = my_stbtt_print_width(">_");
    auto input_line_offset = strwidth + padding + padding;
    if (x < input_line_offset || y < padding || y > input_line_h - padding) return false;

    return true;
}

bool GetColWidthHandle(
    int x,
    int y,
    int &out_col)
{
    x -= header_w;

    if (x < 0 || y < input_line_h || y > (input_line_h + header_h))
    {
        return false;
    }

    auto cols_map = select_into_map("SELECT col_index, size FROM cols");

    int col = scroll_cols;
    int sum_colwidths = 0;

    while (true)
    {
        auto colw = cols_map.find(col);
        if (colw != cols_map.end())
        {
            sum_colwidths += defaultcell_w + colw->second;
        }
        else
        {
            sum_colwidths += defaultcell_w;
        }

        if (std::abs(x - sum_colwidths) < 4)
        {
            out_col = col;
            return true;
        }

        if (x - sum_colwidths < 0)
        {
            break;
        }

        col++;
    }

    return false;
}

bool GetRowHeightHandle(
    int x,
    int y,
    int &out_row)
{
    y -= input_line_h;
    y -= header_h;

    if (y < 0 || x < 0 || x > header_w)
    {
        return false;
    }

    auto rows_map = select_into_map("SELECT row_index, size FROM rows");

    int row = scroll_rows;
    int sum_rowheights = 0;

    while (true)
    {
        auto size = rows_map.find(row);
        if (size != rows_map.end())
        {
            sum_rowheights += defaultcell_h + size->second;
        }
        else
        {
            sum_rowheights += defaultcell_h;
        }

        if (std::abs(y - sum_rowheights) < 4)
        {
            out_row = row;
            return true;
        }

        if (y - sum_rowheights < 0)
        {
            break;
        }

        row++;
    }

    return false;
}

bool GetCellFromScreenPos(
    int x,
    int y,
    int &out_col,
    int &out_row)
{
    x -= header_w;
    y -= input_line_h;
    y -= header_h;

    if (x < 0 || y < 0)
    {
        return false;
    }

    auto cols_map = select_into_map("SELECT col_index, size FROM cols");
    auto rows_map = select_into_map("SELECT row_index, size FROM rows");

    int col = scroll_cols;
    int row = scroll_rows;

    while (true)
    {
        auto colw = cols_map.find(col);
        if (colw != cols_map.end())
        {
            x -= defaultcell_w + colw->second;
        }
        else
        {
            x -= defaultcell_w;
        }

        if (x < 0)
        {
            break;
        }

        col++;
    }

    while (true)
    {
        auto rowh = rows_map.find(row);
        if (rowh != rows_map.end())
        {
            y -= defaultcell_h + rowh->second;
        }
        else
        {
            y -= defaultcell_h;
        }

        if (y < 0)
        {
            break;
        }

        row++;
    }

    out_col = col;
    out_row = row;

    return true;
}

void ChangeColWidth(
    int col,
    int offset)
{
    auto newOffset = db->execute_value<int>("SELECT size FROM cols WHERE col_index = ?", col) + offset;

    if (defaultcell_w + newOffset < 0)
    {
        newOffset = -(defaultcell_w - 5);
    }

    db->execute(R"(REPLACE INTO cols (col_index, size) VALUES (?, ?);)", col, newOffset);
}

void ChangeRowHeight(
    int row,
    int offset)
{
    auto newOffset = db->execute_value<int>("SELECT size FROM rows WHERE row_index = ?", row) + offset;

    if (defaultcell_h + newOffset < 0)
    {
        newOffset = -(defaultcell_h - 5);
    }

    db->execute(R"(REPLACE INTO rows (row_index, size) VALUES (?, ?);)", row, newOffset);
}

static int colDragging = -1;
static int colDraggingX = -1;
static int colDraggingStartX = -1;
static int rowDragging = -1;
static int rowDraggingY = -1;
static int rowDraggingStartY = -1;

void MouseButtonCallback(
    GLFWwindow *window,
    int button,
    int action,
    int mods)
{
    (void)button;
    (void)mods;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (action == GLFW_PRESS)
    {
        int col, row;
        if (GetCellFromScreenPos(x, y, col, row))
        {
            active_cell_col = col;
            active_cell_row = row;
            return;
        }
        else if (GetColWidthHandle(x, y, col))
        {
            colDragging = col;
            colDraggingX = x;
            colDraggingStartX = x;
            return;
        }
        else if (GetRowHeightHandle(x, y, row))
        {
            rowDragging = row;
            rowDraggingY = y;
            rowDraggingStartY = y;

            return;
        }
    }

    if (action == GLFW_RELEASE)
    {
        if (colDragging >= 0)
        {
            ChangeColWidth(colDragging, colDraggingX - colDraggingStartX);
        }
        colDragging = -1;
        colDraggingX = -1;
        colDraggingStartX = -1;

        if (rowDragging >= 0)
        {
            ChangeRowHeight(rowDragging, rowDraggingY - rowDraggingStartY);
        }
        rowDragging = -1;
        rowDraggingY = -1;
        rowDraggingStartY = -1;
    }
}

static GLFWcursor *colSizeCursor = nullptr;
static GLFWcursor *rowSizeCursor = nullptr;
static GLFWcursor *inputLineCursor = nullptr;

void CursorPosCallback(
    GLFWwindow *window,
    double x,
    double y)
{
    (void)window;
    (void)x;
    (void)y;

    if (colDragging >= 0)
    {
        colDraggingX = x;
    }
    else if (rowDragging >= 0)
    {
        rowDraggingY = y;
    }
    else
    {
        int col, row;
        if (GetColWidthHandle(x, y, col))
        {
            if (colSizeCursor == nullptr)
            {
                colSizeCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
            }

            glfwSetCursor(window, colSizeCursor);

            return;
        }
        else if (GetRowHeightHandle(x, y, row))
        {
            if (rowSizeCursor == nullptr)
            {
                rowSizeCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
            }

            glfwSetCursor(window, rowSizeCursor);

            return;
        }
        else if (IsHoveringInputLine(x, y))
        {
            if (inputLineCursor == nullptr)
            {
                inputLineCursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
            }

            glfwSetCursor(window, inputLineCursor);
        }
        else
        {
            glfwSetCursor(window, nullptr);
        }
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
    col_index INTEGER  PRIMARY KEY,
    size INTEGER
  )
)");

        db->execute(R"(
  CREATE TABLE IF NOT EXISTS rows (
    row_index INTEGER PRIMARY KEY,
    size INTEGER
  )
)");

        db->execute(R"(
  CREATE TABLE IF NOT EXISTS sheets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT
  )
)");

        db->execute(R"(DELETE FROM cells;)");
        db->execute(R"(INSERT INTO cells (col, row, tmp_value) VALUES (0, 0, '0');)");
        db->execute(R"(INSERT INTO cells (col, row, tmp_value) VALUES (1, 1, '1');)");
        db->execute(R"(INSERT INTO cells (col, row, tmp_value) VALUES (3, 6, 'test');)");
        db->execute(R"(INSERT INTO cells (col, row, tmp_value) VALUES (2, 8, 'fout');)");
    }
    catch (const std::exception &ex)
    {
        std::cout << db->errormsg() << std::endl;
    }

    return db;
}

#define MAX

void renderSheet(
    std::unique_ptr<sqlitelib::Sqlite> &db,
    int atx,
    int aty)
{
    glDisable(GL_DEPTH_TEST);
    try
    {
        glViewport(atx, 0, w - atx, h - aty);

        auto cols_map = select_into_map("SELECT col_index, size FROM cols");
        auto rows_map = select_into_map("SELECT row_index, size FROM rows");

        int x = header_w, y = input_line_h + header_h + 1;
        glBegin(GL_QUADS);
        glColor3f(0.85f, 0.85f, 0.85f);

        glVertex2f(0.0f, 0.0f);
        glVertex2f(w, 0.0f);
        glVertex2f(w, input_line_h + header_h);
        glVertex2f(0.0f, input_line_h + header_h);

        glVertex2f(0.0f, input_line_h);
        glVertex2f(header_w, input_line_h);
        glVertex2f(header_w, input_line_h + h);
        glVertex2f(0.0f, input_line_h + h);

        glColor3f(1.0f, 1.0f, 1.0f);

        // Render input line
        auto strwidth = my_stbtt_print_width(">_");
        auto input_line_offset = strwidth + padding + padding;
        glVertex2f(input_line_offset, padding);
        glVertex2f(w, padding);
        glVertex2f(w, input_line_h - padding);
        glVertex2f(input_line_offset, input_line_h - padding);
        glEnd();

        my_stbtt_print(
            padding,
            (input_line_h + padding) / 2.0f,
            ">_",
            glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));

        auto tmp_value = db->execute_value<std::string>("SELECT tmp_value FROM cells WHERE col = ? and row = ?", active_cell_col, active_cell_row);

        my_stbtt_print(
            input_line_offset + padding,
            (input_line_h + padding) / 2.0f,
            tmp_value,
            glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));

        glBegin(GL_LINES);
        glColor3f(0.79f, 0.79f, 0.79f);
        auto selected_x = 0, selected_y = 0;
        auto selected_w = 0, selected_h = 0;

        // Render col lines
        int i = scroll_cols;
        while (x < w)
        {
            if (i == active_cell_col)
            {
                selected_x = x;
                selected_w = defaultcell_w;
            }

            glVertex2f(float(x), input_line_h);
            glVertex2f(float(x), float(h));

            auto colw = cols_map.find(i);
            if (colw != cols_map.end())
            {
                if (i == active_cell_col)
                {
                    selected_w = defaultcell_w + colw->second;
                }

                x += defaultcell_w + colw->second;
            }
            else
            {
                x += defaultcell_w;
            }

            i++;
        }

        if (colDraggingX >= 0)
        {
            glColor3f(0.3f, 0.3f, 0.3f);
            glVertex2f(float(colDraggingX), input_line_h);
            glVertex2f(float(colDraggingX), float(h));
            glColor3f(0.79f, 0.79f, 0.79f);
        }

        // Render row lines
        i = scroll_rows;
        while (y < h)
        {
            if (i == active_cell_row)
            {
                selected_y = y;
                selected_h = defaultcell_h;
            }

            glVertex2f(0.0f, float(y));
            glVertex2f(float(w), float(y));

            auto rowh = rows_map.find(i);
            if (rowh != rows_map.end())
            {
                if (i == active_cell_row)
                {
                    selected_h = defaultcell_h + rowh->second;
                }

                y += defaultcell_h + rowh->second;
            }
            else
            {
                y += defaultcell_h;
            }

            i++;
        }

        if (rowDraggingY >= 0)
        {
            glColor3f(0.3f, 0.3f, 0.3f);
            glVertex2f(0.0f, float(rowDraggingY));
            glVertex2f(float(w), float(rowDraggingY));
            glColor3f(0.79f, 0.79f, 0.79f);
        }

        glEnd();

        // Render col names
        i = scroll_cols, x = header_w, y = input_line_h;
        while (x < w)
        {
            auto fromx = x;

            auto colw = cols_map.find(i);
            if (colw != cols_map.end())
            {
                x += defaultcell_w + colw->second;
            }
            else
            {
                x += defaultcell_w;
            }

            i++;

            auto fpsstr = columnIndexToLetters(i);
            auto strwidth = my_stbtt_print_width(fpsstr);

            my_stbtt_print(
                fromx + ((x - fromx) / 2.0f) - (strwidth / 2.0f),
                input_line_h + fontSize * 1.2f,
                fpsstr,
                glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        // Render row #
        i = scroll_rows, x = 0, y = input_line_h + header_h;
        while (y < h)
        {
            glVertex2f(0.0f, float(y));
            glVertex2f(float(w), float(y));

            auto fromy = y;
            auto rowh = rows_map.find(i);
            if (rowh != rows_map.end())
            {
                y += defaultcell_h + rowh->second;
            }
            else
            {
                y += defaultcell_h;
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

        // Select all cells in view
        auto cells = db->execute<int, int, std::string>(
            "SELECT col, row, tmp_value FROM cells WHERE col BETWEEN ? AND ? AND row BETWEEN ? AND ?",
            scroll_cols,
            scroll_cols + max_visible_col_count,
            scroll_rows,
            scroll_rows + max_visible_row_count);

        // Render all cells in view
        for (auto const &cell : cells)
        {
            const int col = std::get<0>(cell);
            const int row = std::get<1>(cell);
            const std::string value = std::get<2>(cell);

            int cell_x = (col * defaultcell_w) + db->execute_value<int>("SELECT SUM(size) FROM cols WHERE col_index < ?", col);
            int scroll_x = (scroll_cols * defaultcell_w) + db->execute_value<int>("SELECT SUM(size) FROM cols WHERE col_index < ?", scroll_cols);
            int cell_y = (row * defaultcell_h) + db->execute_value<int>("SELECT SUM(size) FROM rows WHERE row_index < ?", row);
            int scroll_y = (scroll_rows * defaultcell_h) + db->execute_value<int>("SELECT SUM(size) FROM rows WHERE row_index < ?", scroll_rows);

            my_stbtt_print(
                header_w + cell_x - scroll_x + cell_padding,
                input_line_h + header_h + cell_y - scroll_y + fontSize * 1.2f,
                value,
                glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        }

        // Render selected col header
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.4f, 0.55f, 0.65f);
        glVertex2f(selected_x, input_line_h);
        glVertex2f(selected_x + selected_w, input_line_h);
        glVertex2f(selected_x + selected_w, input_line_h + header_h);
        glVertex2f(selected_x, input_line_h + header_h);
        glEnd();

        {
            auto fpsstr = columnIndexToLetters(active_cell_col + 1);
            auto strwidth = my_stbtt_print_width(fpsstr);

            my_stbtt_print(
                selected_x + (selected_w / 2.0f) - (strwidth / 2.0f),
                input_line_h + fontSize * 1.2f,
                fpsstr,
                glm::vec4(1.0f, 1.0f, 1.0, 1.0f));
        }

        // Render selected row header
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.4f, 0.55f, 0.65f);
        glVertex2f(0, selected_y);
        glVertex2f(header_w, selected_y);
        glVertex2f(header_w, selected_y + selected_h);
        glVertex2f(0, selected_y + selected_h);
        glEnd();

        {
            auto fpsstr = fmt::format("{}", active_cell_row + 1);
            auto strwidth = my_stbtt_print_width(fpsstr);

            my_stbtt_print(
                (header_w / 2.0f) - (strwidth / 2.0f),
                selected_y + (fontSize * 1.2f),
                fpsstr,
                glm::vec4(1.0f, 1.0f, 1.0, 1.0f));
        }

        // Render selected cell
        glLineWidth(1.0f);
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
    (void)argc;
    (void)argv;

    db = InitDb();

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

    EnsureSelectionInView();

    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

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
            w - my_stbtt_print_width(fpsstr) - (fontSize * 0.4f) - 30,
            (input_line_h + padding) / 2.0f,
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

    if (colSizeCursor != nullptr)
    {
        glfwDestroyCursor(colSizeCursor);
    }

    if (rowSizeCursor != nullptr)
    {
        glfwDestroyCursor(rowSizeCursor);
    }

    if (inputLineCursor != nullptr)
    {
        glfwDestroyCursor(inputLineCursor);
    }

    glfwTerminate();

    return (EXIT_SUCCESS);
}
