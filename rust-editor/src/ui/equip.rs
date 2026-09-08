use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;
use crate::save::*;

/// 检查字段是否被选中
fn is_field_selected(app: &EditorApp, path: &[&str]) -> bool {
    if let Some(field) = app.edit_fields.get(app.selected_field) {
        let field_path: Vec<&str> = field.path.iter().map(|s| s.as_str()).collect();
        return field_path == path;
    }
    false
}

/// 格式化字段值 (高亮选中字段)
fn format_field_value(app: &EditorApp, path: &[&str], value: i64) -> (String, Style) {
    let val_str = if value == 0 { "-".to_string() } else { value.to_string() };
    let is_selected = is_field_selected(app, path);
    
    if app.editing && is_selected {
        (app.input_buffer.clone(), Style::default().fg(Color::Black).bg(Color::Yellow))
    } else if is_selected {
        (val_str, Style::default().fg(Color::Black).bg(Color::Cyan))
    } else {
        (val_str, Style::default().fg(Color::Yellow))
    }
}

/// 绘制装备/道具/技能页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(8),  // 装备
            Constraint::Length(5),  // 道具
            Constraint::Length(10), // 技能
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
        let equips = entity.get("EquipIDs").and_then(|v| v.as_object());

        for (slot_id, label) in [("0", "武器(0)"), ("1", "防具(1)"), ("2", "饰品1(2)"), ("3", "头盔(3)"), ("4", "饰品2(4)")] {
            let value = equips
                .and_then(|ids| ids.get(slot_id))
                .and_then(|v| v.as_i64())
                .unwrap_or(0);
            let (val_str, style) = format_field_value(app, &["EquipIDs", slot_id], value);

            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled(
                    format!("{}: ", label),
                    Style::default().fg(Color::White),
                ),
                ratatui::text::Span::styled(val_str, style),
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
        let items = entity.get("ItemIDs")
            .and_then(|v| v.as_array())
            .map(|arr| {
                arr.iter()
                    .filter_map(|v| v.as_i64())
                    .map(|id| id.to_string())
                    .collect::<Vec<_>>()
                    .join(",")
            })
            .unwrap_or_default();

        // 检查是否选中道具字段
        let is_selected = app.edit_fields.get(app.selected_field)
            .map(|f| f.path == vec!["ItemIDs_str".to_string()])
            .unwrap_or(false);

        let (display_val, style) = if app.editing && is_selected {
            (app.input_buffer.clone(), Style::default().fg(Color::Black).bg(Color::Yellow))
        } else if is_selected {
            (items.clone(), Style::default().fg(Color::Black).bg(Color::Cyan))
        } else {
            (if items.is_empty() { "无".to_string() } else { items }, Style::default().fg(Color::Yellow))
        };

        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("背包道具: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(display_val, style),
        ]));
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "格式: 逗号分隔的ID, 如 212,217,588",
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
    let items_widget = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title),
    );

    f.render_widget(items_widget, area);
}

/// 绘制技能
fn draw_skills(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(entity) = app.current_entity() {
        // 普攻技能
        let nrl_skill = get_i64(entity, "NrlSkillID").unwrap_or(0);
        let (nrl_val, nrl_style) = format_field_value(app, &["NrlSkillID"], nrl_skill);
        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("普攻技能ID: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(nrl_val, nrl_style),
        ]));

        // 魔法技能
        let magic_skills = entity.get("MagicSkillIDs")
            .and_then(|v| v.as_array())
            .map(|arr| arr.iter().filter_map(|v| v.as_i64()).collect::<Vec<_>>())
            .unwrap_or_default();

        for i in 0..3 {
            let value = magic_skills.get(i).copied().unwrap_or(0);
            let (val_str, style) = format_field_value(app, &["MagicSkillIDs", &i.to_string()], value);
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled(
                    format!("魔法技能{}: ", i + 1),
                    Style::default().fg(Color::White),
                ),
                ratatui::text::Span::styled(val_str, style),
            ]));
        }

        // 特殊技能
        let sp_skills = entity.get("SpSkillIDs")
            .and_then(|v| v.as_array())
            .map(|arr| arr.iter().filter_map(|v| v.as_i64()).collect::<Vec<_>>())
            .unwrap_or_default();

        for i in 0..2 {
            let value = sp_skills.get(i).copied().unwrap_or(0);
            let (val_str, style) = format_field_value(app, &["SpSkillIDs", &i.to_string()], value);
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled(
                    format!("特殊技能{}: ", i + 1),
                    Style::default().fg(Color::White),
                ),
                ratatui::text::Span::styled(val_str, style),
            ]));
        }
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
