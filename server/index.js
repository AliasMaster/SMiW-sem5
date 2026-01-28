const path = require('path');
require('dotenv').config();

const app = express();
const port = 3000;

app.use(cors());
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));

const API_KEY = process.env.API_KEY;

// Middleware do sprawdzania klucza API
const apiKeyMiddleware = (req, res, next) => {
    const key = req.header('X-API-Key');
    if (key && key === API_KEY) {
        next();
    } else {
        res.status(401).send('Brak autoryzacji: Niepoprawny klucz API');
    }
};

// Zabezpieczenie wszystkich endpointów /api
app.use('/api', apiKeyMiddleware);

// Inicjalizacja bazy danych
const db = new sqlite3.Database('./database.sqlite', (err) => {
    if (err) {
        console.error('Błąd otwierania bazy danych:', err.message);
    } else {
        console.log('Połączono z bazą danych SQLite.');
        db.run(`CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            temp REAL,
            threshold REAL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )`);
        
        db.run(`CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT
        )`, () => {
            db.run(`INSERT OR IGNORE INTO settings (key, value) VALUES ('threshold', '30.0')`);
        });
    }
});

// Endpoint dla ESP8266
app.post('/api/data', (req, res) => {
    const { temp, threshold } = req.body;
    
    if (temp === undefined) {
        return res.status(400).send('Brak danych temperatury');
    }

    // Zapisz do historii
    db.run(`INSERT INTO history (temp, threshold) VALUES (?, ?)`, [temp, threshold], (err) => {
        if (err) {
            console.error('Błąd zapisu historii:', err.message);
        }
    });

    // Pobierz aktualny próg z bazy danych
    db.get(`SELECT value FROM settings WHERE key = 'threshold'`, (err, row) => {
        if (err) {
            res.status(500).send(err.message);
        } else {
            res.json({ threshold: parseFloat(row.value) });
        }
    });
});

// Endpoint dla frontendu - pobieranie historii
app.get('/api/history', (req, res) => {
    const range = req.query.range || 'day';
    
    db.get(`SELECT value FROM settings WHERE key = 'threshold'`, (err, setting) => {
        if (err) return res.status(500).send(err.message);
        
        let query = "";
        let limit = 2880; // Standardowo ok. 2 dni przy 1-minutowych odstępach

        if (range === 'day') {
            // Ostatnie 24h, bez agregacji (wszystkie punkty)
            query = `SELECT temp, threshold, timestamp FROM history 
                     WHERE timestamp > datetime('now', '-1 day') 
                     ORDER BY timestamp DESC`;
        } else if (range === 'month') {
            // Ostatnie 30 dni, agregacja co godzinę
            query = `SELECT AVG(temp) as temp, AVG(threshold) as threshold, 
                     strftime('%Y-%m-%d %H:00:00', timestamp) as timestamp 
                     FROM history 
                     WHERE timestamp > datetime('now', '-30 days')
                     GROUP BY strftime('%Y-%m-%d %H', timestamp)
                     ORDER BY timestamp DESC`;
        } else if (range === 'year') {
            // Ostatni rok, agregacja co dzień
            query = `SELECT AVG(temp) as temp, AVG(threshold) as threshold, 
                     strftime('%Y-%m-%d', timestamp) as timestamp 
                     FROM history 
                     WHERE timestamp > datetime('now', '-1 year')
                     GROUP BY strftime('%Y-%m-%d', timestamp)
                     ORDER BY timestamp DESC`;
        } else {
            // Domyślnie ostatnie punkty
            query = `SELECT temp, threshold, timestamp FROM history ORDER BY timestamp DESC LIMIT 2880`;
        }
        
        db.all(query, (err, rows) => {
            if (err) {
                res.status(500).send(err.message);
            } else {
                // Dodatkowo pobierz absolutnie ostatnią zapisaną temperaturę (dla widoku real-time)
                db.get(`SELECT temp FROM history ORDER BY timestamp DESC LIMIT 1`, (err, latest) => {
                    res.json({
                        history: rows.reverse(),
                        currentThreshold: parseFloat(setting.value),
                        latestTemp: latest ? latest.temp : null
                    });
                });
            }
        });
    });
});

// Endpoint dla frontendu - zmiana progu
app.post('/api/threshold', (req, res) => {
    const { value } = req.body;
    if (value === undefined) {
        return res.status(400).send('Brak wartości progu');
    }

    db.run(`UPDATE settings SET value = ? WHERE key = 'threshold'`, [value], (err) => {
        if (err) {
            res.status(500).send(err.message);
        } else {
            res.send('OK');
        }
    });
});

app.listen(port, () => {
    console.log(`Serwer działa pod adresem http://localhost:${port}`);
});
