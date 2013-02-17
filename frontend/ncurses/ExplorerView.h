#ifndef EXPLORERVIEW_H
#define EXPLORERVIEW_H

#include "IView.h"

#include <string>
#include <vector>
#include <deque>
#include <set>

#include <scx/Signal.hpp>
#include <scx/UniPinYin.hpp>

class FileItemCmp;

class ExplorerView: public IView
{
    friend class FileItemCmp;

public:
    ExplorerView();
    ~ExplorerView();

    void Refresh();
    void MoveTo(int x, int y);
    void Resize(int w, int h);

    bool InjectKey(int key);

    void Show(bool show);
    bool IsShown() const;

    void SetFocus(bool focus);
    bool HasFocus() const;

public:
    scx::Signal<void (const std::string&)> SigTmpOpen;
    scx::Signal<void (const std::string&)> SigUserOpen;

    void SetSuffixes(const std::vector<std::string>&);

private:
    void BuildFileItems();
    void CdUp();
    void CdIn();
    void ScrollUp();
    void ScrollDown();

private:
    struct FileItem
    {
        std::string name;
        bool isDir;
        off_t size;
        mutable bool cacheOk;
        mutable std::string nameCache;
        mutable std::string sizeCache;
    };

private:
    Window d;
    bool m_Focused;
    std::string m_Path;
    std::string m_PathCache;
    bool m_HideDot;
    bool m_HideUnknown;
    std::deque<int> m_BeginStack;
    std::deque<int> m_SelectionStack;
    std::vector<FileItem> m_FileItems;
    scx::UniPinYin m_UniPinYin;
    std::set<std::string> m_Suffixes;
};

#endif
