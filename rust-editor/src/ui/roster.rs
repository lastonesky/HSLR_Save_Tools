use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::widgets::{Block, Borders, Cell, Row, Table};

use crate::editor::EditorApp;

/// 绘制全角色一览页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Min(10),  // 角色表格
            Constraint::Length(3), // 说明
        ])
        .split(area);

    // 角色表格
    draw_roster_table(f, app, chunks[0]);

    // 说明
    draw_note(f, chunks[1]);
}

/// 绘制角色表格
fn draw_roster_table(f: &mut Frame, app: &EditorApp, area: Rect) {
    let header_cells = ["ID", "名称", "等级", "HP", "MaxHP", "物攻", "魔攻", "防御", "阵营"]
        .iter()
        .map(|h| Cell::from(*h).style(Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD)));
    let header = Row::new(header_cells).height(1);

    let mut rows = Vec::new();

    if let Some(ref stage) = app.stage {
        if let Some(ref entities_map) = stage.char_entities_map {
            for (key, entity_value) in entities_map {
                if let Ok(entity) = crate::save::parse_entity(entity_value) {
                    let pid = entity.player_id.map_or("?".to_string(), |v| v.to_string());
                    let name = entity.name.clone().unwrap_or_else(|| key.clone());
                    let level = entity.level.map_or("?".to_string(), |v| v.to_string());
                    let hp = entity.hp.map_or("?".to_string(), |v| v.to_string());
                    let max_hp = entity.max_hp.map_or("?".to_string(), |v| v.to_string());
                    let phys_atk = entity
                        .fight_attr
                        .as_ref()
                        .and_then(|fa| fa.physical_attack)
                        .map_or("?".to_string(), |v| v.to_string());
                    let mag_atk = entity
                        .fight_attr
                        .as_ref()
                        .and_then(|fa| fa.magic_attack)
                        .map_or("?".to_string(), |v| v.to_string());
                    let defense = entity
                        .fight_attr
                        .as_ref()
                        .and_then(|fa| fa.defense)
                        .map_or("?".to_string(), |v| v.to_string());
                    let camp = entity.camp.map_or("?".to_string(), |v| match v {
                        1 => "敌方".to_string(),
                        2 => "我方".to_string(),
                        3 => "中立".to_string(),
                        _ => v.to_string(),
                    });

                    let row = Row::new(vec![
                        Cell::from(pid),
                        Cell::from(name),
                        Cell::from(level),
                        Cell::from(hp),
                        Cell::from(max_hp),
                        Cell::from(phys_atk),
                        Cell::from(mag_atk),
                        Cell::from(defense),
                        Cell::from(camp),
                    ]);

                    rows.push(row);
                }
            }
        }
    }

    let table = Table::new(
        rows,
        [
            Constraint::Length(6),  // ID
            Constraint::Length(16), // 名称
            Constraint::Length(6),  // 等级
            Constraint::Length(8),  // HP
            Constraint::Length(8),  // MaxHP
            Constraint::Length(8),  // 物攻
            Constraint::Length(8),  // 魔攻
            Constraint::Length(8),  // 防御
            Constraint::Length(8),  // 阵营
        ],
    )
    .header(header)
    .block(Block::default().borders(Borders::ALL).title("全角色一览"))
    .highlight_style(Style::default().bg(Color::DarkGray).add_modifier(Modifier::BOLD))
    .highlight_symbol(">> ");

    f.render_widget(table, area);
}

/// 绘制说明
fn draw_note(f: &mut Frame, area: Rect) {
    let note = ratatui::widgets::Paragraph::new(
        "💡 双击单元格可进行内联编辑 (当前TUI版本暂不支持，需使用完整编辑器)",
    )
    .style(Style::default().fg(Color::DarkGray))
    .block(Block::default().borders(Borders::ALL).title("说明"));

    f.render_widget(note, area);
}
