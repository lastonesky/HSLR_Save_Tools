pub mod basic;
pub mod battle;
pub mod equip;
pub mod record;
pub mod roster;

use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::widgets::{Block, Borders, Paragraph, Tabs};

use crate::editor::EditorApp;

/// 标签页标题
const TAB_TITLES: [&str; 5] = [
    "基础信息",
    "存档属性",
    "战场属性",
    "装备/道具/技能",
    "全角色一览",
];

/// 绘制主界面
pub fn draw(f: &mut Frame, app: &EditorApp) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3),  // 标签页
            Constraint::Min(10),   // 内容区
            Constraint::Length(3), // 状态栏
            Constraint::Length(3), // 快捷键提示
        ])
        .split(f.area());

    // 绘制标签页
    draw_tabs(f, app, chunks[0]);

    // 绘制内容区
    match app.current_tab {
        0 => basic::draw(f, app, chunks[1]),
        1 => record::draw(f, app, chunks[1]),
        2 => battle::draw(f, app, chunks[1]),
        3 => equip::draw(f, app, chunks[1]),
        4 => roster::draw(f, app, chunks[1]),
        _ => {}
    }

    // 绘制状态栏
    draw_status_bar(f, app, chunks[2]);

    // 绘制快捷键提示
    draw_help_bar(f, chunks[3]);
}

/// 绘制标签页
fn draw_tabs(f: &mut Frame, app: &EditorApp, area: Rect) {
    let titles: Vec<ratatui::text::Line> = TAB_TITLES
        .iter()
        .enumerate()
        .map(|(i, t)| {
            let style = if i == app.current_tab {
                Style::default()
                    .fg(Color::Yellow)
                    .add_modifier(Modifier::BOLD)
            } else {
                Style::default().fg(Color::White)
            };
            ratatui::text::Line::from(ratatui::text::Span::styled(*t, style))
        })
        .collect();

    let tabs = Tabs::new(titles)
        .block(Block::default().borders(Borders::ALL).title("幻世录重制版 存档编辑器 v2.0"))
        .select(app.current_tab)
        .style(Style::default().fg(Color::White))
        .highlight_style(Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD));

    f.render_widget(tabs, area);
}

/// 绘制状态栏
fn draw_status_bar(f: &mut Frame, app: &EditorApp, area: Rect) {
    let style = if app.status_is_error {
        Style::default().fg(Color::Red)
    } else {
        Style::default().fg(Color::Green)
    };

    let status = Paragraph::new(app.status_message.clone())
        .style(style)
        .block(Block::default().borders(Borders::ALL).title("状态"));

    f.render_widget(status, area);
}

/// 绘制快捷键提示
fn draw_help_bar(f: &mut Frame, area: Rect) {
    let help_text = "Tab/Shift+Tab: 切换标签页 | F2: 全队满属性 | F3: 保存 | F5: 刷新 | Q: 退出";
    let help = Paragraph::new(help_text)
        .style(Style::default().fg(Color::DarkGray))
        .block(Block::default().borders(Borders::ALL).title("快捷键"));

    f.render_widget(help, area);
}
