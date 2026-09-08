use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;

/// 绘制战场属性页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(12), // 战场属性
            Constraint::Length(5),  // 快捷操作
            Constraint::Length(4),  // 警告
        ])
        .split(area);

    // 战场属性
    draw_battle_attrs(f, app, chunks[0]);

    // 快捷操作
    draw_quick_actions(f, chunks[1]);

    // 警告
    draw_warning(f, chunks[2]);
}

/// 绘制战场属性
fn draw_battle_attrs(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(entity) = app.current_entity() {
        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("当前HP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.hp.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("最大HP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.max_hp.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("当前MP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.mp.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("最大MP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.max_mp.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
        ]));

        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("等级: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.level.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("经验: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.exp.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
        ]));

        if let Some(ref fight_attr) = entity.fight_attr {
            lines.push(ratatui::text::Line::from(""));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.str.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("敏捷: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.dex.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("智力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.mind.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("体质: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.con.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("物攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.physical_attack.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("魔攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.magic_attack.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("防御: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.defense.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("速度: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.speed.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("移动力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.move_range.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("暴击率: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.critical_ratio.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("闪避率: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fight_attr.dodge_ratio.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
        }
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "战场属性 (charEntitiesMap)";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title),
    );

    f.render_widget(attr, area);
}

/// 绘制快捷操作
fn draw_quick_actions(f: &mut Frame, area: Rect) {
    let actions = vec![
        "H: 一键满血满蓝",
        "M: 战场属性MAX(本战)",
        "L: Lv99 + 满经验",
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

/// 绘制警告
fn draw_warning(f: &mut Frame, area: Rect) {
    let warning = Paragraph::new("⚠ 战场属性仅当前战斗有效，下次进图会重算。\n永久修改请到「存档属性」页或用底部「全队满属性(持久)」按钮。")
        .style(Style::default().fg(Color::Rgb(176, 96, 0)))
        .block(Block::default().borders(Borders::ALL).title("警告"));

    f.render_widget(warning, area);
}
