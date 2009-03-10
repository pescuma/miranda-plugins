package org.pescuma.miricogen;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.pescuma.jfg.gui.swt.JfgFormComposite;
import org.pescuma.jfg.gui.swt.JfgFormData;
import org.pescuma.jfg.reflect.ReflectionGroup;

public class ImportSkinIcons
{
	private static final Pattern groupPattern = Pattern.compile("\\s*\\[([^\\]]+)\\]\\s*");
	private static final Pattern entryPattern = Pattern.compile("([^=]+)=[ua]([^,]+)(,[0-9]+)?");
	
	private Config cfg = new Config();
	private Text out;
	
	public class Config
	{
		public File iniFile;
		public File mirandaPath;
		public File pluginsPath = new File("plugins");
	}
	
	public void show(Shell parent, File pluginsPath)
	{
		cfg.pluginsPath = pluginsPath;
		
		Display display = parent.getDisplay();
		final Shell shell = new Shell(parent, SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL);
		shell.setLayout(new GridLayout(1, true));
		
		final JfgFormComposite form = new JfgFormComposite(shell, SWT.NONE, new JfgFormData(JfgFormData.DIALOG));
		form.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		form.addContentsFrom(new ReflectionGroup(cfg));
		
		Button ok = new Button(shell, SWT.PUSH);
		ok.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		ok.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event event)
			{
				form.copyToModel();
				
				try
				{
					process();
				}
				catch (Exception e)
				{
					out.append("[ERROR] " + e.getClass().getName() + ": " + e.getMessage() + "\n\n\n");
					
					e.printStackTrace();
					MessageBox mb = new MessageBox(shell, SWT.OK | SWT.ERROR);
					mb.setText("Error");
					mb.setMessage(e.getMessage());
					mb.open();
				}
			}
		});
		ok.setText("Import");
		
		out = new Text(shell, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		out.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		shell.setText("SkinIcons Importer");
		shell.setDefaultButton(ok);
		shell.pack();
		shell.setSize(500, 300);
		shell.open();
		while (!shell.isDisposed())
		{
			if (!display.readAndDispatch())
				display.sleep();
		}
	}
	
	protected void process() throws IOException, InterruptedException
	{
		out.append("Starting process...\n\n");
		
		Map<String, File> db = readINI();
		
		List<Plugin> plugins = Main.readPlugins(false);
		
		copyFiles(plugins, db);
		
		out.append("\n");
		out.append("Done\n\n\n");
	}
	
	private void copyFiles(List<Plugin> plugins, Map<String, File> db)
	{
		for (Plugin plugin : plugins)
		{
			for (PluginIcon icon : plugin.getIcons())
			{
				File file = db.get(icon.getName().toLowerCase());
				if (file == null)
					continue;
				
				try
				{
					Main.copy(icon.getFile(), file);
				}
				catch (IOException e)
				{
					out.append("[ERROR] Error copying file " + file.getAbsolutePath() + ": " + e.getClass().getName() + ": "
							+ e.getMessage() + "\n");
					e.printStackTrace();
				}
			}
		}
	}
	
	private Map<String, File> readINI()
	{
		if (!cfg.iniFile.exists())
			throw new IllegalStateException(".ini file don't exist: " + cfg.iniFile.getAbsolutePath());
		
		BufferedReader in = null;
		try
		{
			Map<String, File> ret = new HashMap<String, File>();
			String group = "";
			
			in = new BufferedReader(new FileReader(cfg.iniFile));
			String line;
			while ((line = in.readLine()) != null)
			{
				line = line.trim();
				if (line.isEmpty())
					continue;
				
				Matcher m = groupPattern.matcher(line);
				if (m.matches())
				{
					group = m.group(1);
					continue;
				}
				
				m = entryPattern.matcher(line);
				if ("SkinIcons".equals(group) && m.matches())
				{
					String name = m.group(1).trim().toLowerCase();
					File f = new File(cfg.mirandaPath, m.group(2).trim());
					ret.put(name, f);
					continue;
				}
			}
			
			return ret;
		}
		catch (IOException e)
		{
			throw new RuntimeException("Error loading .ini file", e);
		}
		finally
		{
			if (in != null)
			{
				try
				{
					in.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}
}
