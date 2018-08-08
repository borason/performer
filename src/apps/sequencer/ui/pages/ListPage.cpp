#include "ListPage.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"

#include "core/math/Math.h"

static void drawScrollbar(Canvas &canvas, int x, int y, int w, int h, int totalRows, int visibleRows, int displayRow) {
    if (visibleRows >= totalRows) {
        return;
    }

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(0x7);
    canvas.drawRect(x, y, w, h);

    int bh = (visibleRows * h) / totalRows;
    int by = (displayRow * h) / totalRows;
    canvas.setColor(0xf);
    canvas.fillRect(x, y + by, w, bh);
}

ListPage::ListPage(PageManager &manager, PageContext &context, ListModel &listModel) :
    BasePage(manager, context)
{
    setListModel(listModel);
}

void ListPage::setListModel(ListModel &listModel) {
    _listModel = &listModel;
    setSelectedRow(0);
    _edit = false;
}

void ListPage::show() {
    BasePage::show();
}

void ListPage::enter() {
    resetKeyState();

    scrollTo(_selectedRow);
}

void ListPage::exit() {
}

void ListPage::draw(Canvas &canvas) {
    int displayRow = _displayRow;
    if (displayRow + LineCount > _listModel->rows()) {
        displayRow = std::max(0, _listModel->rows() - LineCount);
    }

    canvas.setFont(Font::Small);
    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(0xf);

    for (int i = 0; i < LineCount; ++i) {
        int row = displayRow + i;
        if (row < _listModel->rows()) {
            drawCell(canvas, row, 0, 8, 12 + i * LineHeight, 128 - 16, LineHeight);
            drawCell(canvas, row, 1, 128, 12 + i * LineHeight, 128 - 16, LineHeight);
        }
    }

    drawScrollbar(canvas, Width - 8, 12, 4, LineCount * LineHeight, _listModel->rows(), LineCount, displayRow);
}

void ListPage::updateLeds(Leds &leds) {
    LedPainter::drawStepIndex(leds,_listModel->indexed(_selectedRow));
}

void ListPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.pageModifier()) {
        return;
    }

    if (key.isLeft()) {
        if (_edit) {
            _listModel->edit(_selectedRow, 1, -1, globalKeyState()[Key::Shift]);
        } else {
            setSelectedRow(selectedRow() - 1);
        }
        event.consume();
    }
    else if (key.isRight()) {
        if (_edit) {
            _listModel->edit(_selectedRow, 1, 1, globalKeyState()[Key::Shift]);
        } else {
            setSelectedRow(selectedRow() + 1);
        }
        event.consume();
    } else if (key.isEncoder()) {
        _edit = !_edit;
        event.consume();
    } else if (key.isStep()) {
        _listModel->setIndexed(_selectedRow, key.step());
        event.consume();
    }
}

void ListPage::encoder(EncoderEvent &event) {
    if (_edit) {
        _listModel->edit(_selectedRow, 1, event.value(), event.pressed() | globalKeyState()[Key::Shift]);
    } else {
        setSelectedRow(selectedRow() + event.value());
    }
    event.consume();
}

void ListPage::setSelectedRow(int selectedRow) {
    if (selectedRow != _selectedRow) {
        _selectedRow = clamp(selectedRow, 0, _listModel->rows() - 1);;
        scrollTo(_selectedRow);
    }
}

void ListPage::setTopRow(int row) {
    _displayRow = std::min(row, std::max(0, _listModel->rows() - LineCount));
}

void ListPage::drawCell(Canvas &canvas, int row, int column, int x, int y, int w, int h) {
    // canvas.drawRect(x, y, w, h);

    FixedStringBuilder<32> str;
    _listModel->cell(row, column, str);
    canvas.setColor(column == int(_edit) && row == _selectedRow ? 0xf : 0x7);
    canvas.drawText(x, y + 7, str);
}

void ListPage::scrollTo(int row) {
    if (row < _displayRow) {
        _displayRow = row;
    } else if (row >= _displayRow + LineCount) {
        _displayRow = row - (LineCount - 1);
    }
}
