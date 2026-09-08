use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;

/// 绘制基础信息页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3),  // 角色选择
            Constraint::Min(5),    // 基础信息
            Constraint::Length(5), // 快捷操作
        ])
        .split(area);

    // 角色选择信息
    draw_char_selector(f, app, chunks[0]);

    // 基础信息
    draw_basic_info(f, app, chunks[1]);

    // 快捷操作说明
    draw_quick_actions(f, chunks[2]);
}

/// 绘制角色选择器
fn draw_char_selector(f: &mut Frame, app: &EditorApp, area: Rect) {
    let char_info = if let Some(pid) = app.current_pid {
        let name = app
            .all_entities
            .get(&pid)
            .and_then(|(_, e)| e.name.clone())
            .unwrap_or_else(|| format!("角色{}", pid));
        let level = app
            .all_entities
            .get(&pid)
            .and_then(|(_, e)| e.level)
            .unwrap_or(0);
        format!("当前角色: {} Lv.{} (PID:{})", name, level, pid)
    } else {
        "请先打开存档文件".to_string()
    };

    let selector = Paragraph::new(char_info)
        .style(Style::default().fg(Color::Cyan))
        .block(Block::default().borders(Borders::ALL).title("角色选择"));

    f.render_widget(selector, area);
}

/// 绘制基础信息
fn draw_basic_info(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("等级: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                record.level.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
            ratatui::text::Span::raw("    "),
            ratatui::text::Span::styled("经验: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                record.exp.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
        ]));

        if let Some(ref base_attr) = record.base_attr {
            lines.push(ratatui::text::Line::from(""));
            lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
                "基础属性 (BaseAttr)",
                Style::default().fg(Color::Green).add_modifier(ratatui::style::Modifier::BOLD),
            )));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.str.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("敏捷: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.dex.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("智力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.mind.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("体质: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.con.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("基础HP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.hp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("基础MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.mp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
        }
    } else {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "请先打开存档文件",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let info = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title("基础信息"),
    );

    f.render_widget(info, area);
}

/// 绘制快捷操作说明
fn draw_quick_actions(f: &mut Frame, area: Rect) {
    let actions = vec![
        "F2: 全队满属性(持久生效，升级不会回缩)",
        "F3: 保存存档",
        "F5: 刷新显示",
        "1-5: 切换标签页",
        "Q/Esc: 退出",
    ];

    let lines: Vec<ratatui::text::Line> = actions
        .iter()
        .map(|a| ratatui::text::Line::from(ratatui::text::Span::styled(*a, Style::default().fg(Color::White))))
        .collect();

    let help = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title("快捷操作"),
    );

    f.render_widget(help, area);
}
