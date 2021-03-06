#pragma once

#include <QtWidgets>

namespace sqt {

static inline void  SetActionSeparator(QList<QAction*> list)
{
    for (int i = 0; i < list.size(); ++i) {
        QAction* action = list[i];
        if (action->text().isEmpty())
            action->setSeparator(true);
    }
}

static inline void AdjustAllStackPages(QStackedWidget* stack, QSizePolicy plicy)
{
    for (int i = 0; i < stack->count(); ++i)
    {
        stack->widget(i)->setSizePolicy(plicy);
    }
}

static inline void SwitchStackPage(QStackedWidget* stack, int index)
{
    const int pageCount = stack->count();
    if (pageCount != 0)
    {
        QSizePolicy policyMin(QSizePolicy::Ignored, QSizePolicy::Ignored);
        QSizePolicy policyMax(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // minimal previous page
        if (stack->currentIndex() != index)
        {
            stack->currentWidget()->setSizePolicy(policyMin);
        }
        else
        {
            // minimal all page
            AdjustAllStackPages(stack, policyMin);
        }
        // show and maximal new page
        stack->setCurrentIndex(index);
        stack->currentWidget()->setSizePolicy(policyMax);
        stack->adjustSize();
    }
}

}

