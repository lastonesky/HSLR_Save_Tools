use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;

/// 绘制存档属性页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3),  // 说明
            Constraint::Length(8),  // BaseAttr
            Constraint::Length(10), // PermanentFightAttr
            Constraint::Min(8),    // FightAttr
        ])
        .split(area);

    // 说明
    draw_note(f, chunks[0]);

    // BaseAttr (基础属性点，永久生效)
    draw_base_attr(f, app, chunks[1]);

    // PermanentFightAttr (永久加成，永久生效)
    draw_permanent_attr(f, app, chunks[2]);

    // FightAttr (战斗属性，只读参考)
    draw_fight_attr(f, app, chunks[3]);
}

/// 绘制说明
fn draw_note(f: &mut Frame, area: Rect) {
    let note = Paragraph::new("💡 永久加成和基础属性会被保留；战斗属性每次加载自动重算")
        .style(Style::default().fg(Color::Blue))
        .block(Block::default().borders(Borders::ALL).title("说明"));

    f.render_widget(note, area);
}

/// 绘制BaseAttr
fn draw_base_attr(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        if let Some(ref base_attr) = record.base_attr {
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量(Str): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.str.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("敏捷(Dex): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.dex.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("智力(Mind): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.mind.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("体质(Con): ", Style::default().fg(Color::White)),
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
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("基础MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    base_attr.mp.map_or("-".to_string(), |v| v.to_string()),
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

    let title = "基础属性 (BaseAttr) ★核心★";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title)
            .title_style(Style::default().fg(Color::Green).add_modifier(Modifier::BOLD)),
    );

    f.render_widget(attr, area);
}

/// 绘制PermanentFightAttr
fn draw_permanent_attr(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        if let Some(ref pfa) = record.permanent_fight_attr {
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.str.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("敏捷: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.dex.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("智力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.mind.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("体质: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.con.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("HP加成: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.max_hp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("MP加成: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.max_mp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("物攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.physical_attack.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("魔攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.magic_attack.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("防御: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.defense.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("速度: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.speed.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("暴击率: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.critical_ratio.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::Yellow),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("闪避率: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    pfa.dodge_ratio.map_or("-".to_string(), |v| v.to_string()),
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

    let title = "永久加成 (PermanentFightAttr) ★核心★";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title)
            .title_style(Style::default().fg(Color::Green).add_modifier(Modifier::BOLD)),
    );

    f.render_widget(attr, area);
}

/// 绘制FightAttr (只读)
fn draw_fight_attr(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        if let Some(ref fa) = record.fight_attr {
            lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
                "战斗属性 (FightAttr)  [自动计算 · 只读参考]",
                Style::default().fg(Color::DarkGray).add_modifier(Modifier::ITALIC),
            )));
            lines.push(ratatui::text::Line::from(""));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("当前HP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.hp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("最大HP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.max_hp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("当前MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.mp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("最大MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.max_mp.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.str.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("敏捷: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.dex.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("智力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.mind.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("体质: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.con.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("物攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.physical_attack.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("魔攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.magic_attack.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("防御: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.defense.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("速度: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    fa.speed.map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
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

    let title = "战斗属性 (FightAttr)  [只读参考]";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title)
            .title_style(Style::default().fg(Color::DarkGray)),
    );

    f.render_widget(attr, area);
}
