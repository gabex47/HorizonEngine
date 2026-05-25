import "./App.css";

const sidebarItems = ["Scene", "Scripts", "Properties"];

function App() {
  return (
    <main className="editor-shell">
      <aside className="sidebar">
        <div className="brand">HorizonEngine</div>
        <nav className="sidebar-nav" aria-label="Editor sections">
          {sidebarItems.map((item) => (
            <button className="sidebar-item" type="button" key={item}>
              {item}
            </button>
          ))}
        </nav>
      </aside>
      <section className="viewport-panel" aria-label="Viewport">
        <div className="viewport" />
      </section>
    </main>
  );
}

export default App;
