use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;

/// 绘制装备/道具/技能页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(6),  // 装备
            Constraint::Length(6),  // 道具
            Constraint::Length(8),  // 技能
        ])
        .split(area);

    // 装备
    draw_equips(f, app, chunks[0]);

    // 道具
    draw_items(f, app, chunks[1]);

    // 技能
    draw_skills(f, app, chunks[2]);
}

/// 绘制装备
fn draw_equips(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(entity) = app.current_entity() {
        let slots = [
            ("武器(0)", "0"),
            ("防具(1)", "1"),
            ("饰品1(2)", "2"),
            ("头盔(3)", "3"),
            ("饰品2(4)", "4"),
        ];

        for (label, slot_id) in slots {
            let value = entity
                .equip_ids
                .as_ref()
                .and_then(|ids| ids.get(slot_id))
                .map_or("-".to_string(), |v| v.to_string());

            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled(
                    format!("{}: ", label),
                    Style::default().fg(Color::White),
                ),
                ratatui::text::Span::styled(value, Style::default().fg(Color::Yellow)),
            ]));
        }
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "装备 (EquipIDs)";
    let equip = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title),
    );

    f.render_widget(equip, area);
}

/// 绘制道具
fn draw_items(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(entity) = app.current_entity() {
        let items = entity.item_ids.as_ref().map_or("".to_string(), |ids| {
            ids.iter()
                .map(|id| id.to_string())
                .collect::<Vec<_>>()
                .join(", ")
        });

        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("背包道具 (ItemIDs): ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                if items.is_empty() { "无".to_string() } else { items },
                Style::default().fg(Color::Yellow),
            ),
        ]));
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "格式: 逗号分隔的ID, 如 212,217",
            Style::default().fg(Color::DarkGray),
        )));
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "背包道具";
    let items = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title),
    );

    f.render_widget(items, area);
}

/// 绘制技能
fn draw_skills(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(entity) = app.current_entity() {
        // 普攻技能
        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("普攻技能ID: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                entity.nrl_skill_id.map_or("-".to_string(), |v| v.to_string()),
                Style::default().fg(Color::Yellow),
            ),
        ]));

        // 魔法技能
        let magic_skills = entity.magic_skill_ids.as_ref().map_or("".to_string(), |ids| {
            ids.iter()
                .map(|id| id.to_string())
                .collect::<Vec<_>>()
                .join(", ")
        });
        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("魔法技能IDs: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                if magic_skills.is_empty() {
                    "无".to_string()
                } else {
                    magic_skills
                },
                Style::default().fg(Color::Yellow),
            ),
        ]));

        // 特殊技能
        let sp_skills = entity.sp_skill_ids.as_ref().map_or("".to_string(), |ids| {
            ids.iter()
                .map(|id| id.to_string())
                .collect::<Vec<_>>()
                .join(", ")
        });
        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("特殊技能IDs: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(
                if sp_skills.is_empty() {
                    "无".to_string()
                } else {
                    sp_skills
                },
                Style::default().fg(Color::Yellow),
            ),
        ]));
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "技能";
    let skills = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title),
    );

    f.render_widget(skills, area);
}
